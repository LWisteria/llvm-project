#include "ShuffleSynthesis.h"

#include <unordered_map>

namespace llvm {

unsigned
GetVectorNumElements(Type *Ty) {
  return cast<FixedVectorType>(Ty)->getNumElements();
}

Type*
GetVectorElementType(Type *Ty) {
  return cast<FixedVectorType>(Ty)->getElementType();
}

VecLenOpt InferLengthFromMask(SDValue MaskV) {
  std::unique_ptr<MaskView> MV(requestMaskView(MaskV.getNode()));
  if (!MV)
    return None;

  unsigned FirstDef, LastDef, NumElems;
  BVMaskKind BVK = AnalyzeBitMaskView(*MV.get(), FirstDef, LastDef, NumElems);
  if (BVK == BVMaskKind::Interval) {
    // FIXME \p FirstDef must be == 0
    return LastDef + 1;
  }

  return None;
}

SDValue ReduceVectorLength(SDValue Mask, SDValue DynamicVL, VecLenOpt VLHint,
                           SelectionDAG &DAG) {
  ConstantSDNode *ActualConstVL = nullptr;
  if (DynamicVL) {
    ActualConstVL = dyn_cast<ConstantSDNode>(DynamicVL);
  }
  if (!ActualConstVL)
    return DynamicVL;

  int64_t EVLVal = ActualConstVL->getSExtValue();
  SDLoc DL(DynamicVL);

  // no hint available -> dynamic VL
  if (!VLHint)
    return DynamicVL;

  // in-effective DynamicVL -> return the hint
  if (EVLVal < 0) {
    return DAG.getConstant(VLHint.getValue(), DL, MVT::i32);
  }

  if (Mask) {
    VecLenOpt MaskVL = InferLengthFromMask(Mask);
    VLHint = MinVectorLength(MaskVL, VLHint);
  }

  // the minimum of dynamicVL and the VLHint
  VecLenOpt MinOfAllHints = MinVectorLength(VLHint, EVLVal);
  return DAG.getConstant(MinOfAllHints.getValue(), DL, MVT::i32);
}

BVMaskKind AnalyzeBitMaskView(MaskView &MV, unsigned &FirstOne,
                              unsigned &FirstZero, unsigned &NumElements) {
  bool HasFirstOne = false, HasFirstZero = false;
  FirstOne = 0;
  FirstZero = 0;
  NumElements = 0;

  // this matches a 0*1*0* pattern (BVMaskKind::Interval)
  for (unsigned i = 0; i < MV.getNumElements(); ++i) {
    auto ES = MV.getSourceElem(i);
    if (!ES.isDefined())
      continue;
    ++NumElements;
    if (!ES.isElemInsert())
      return BVMaskKind::Unknown;

    auto CE = dyn_cast<ConstantSDNode>(ES.V);
    if (!CE)
      return BVMaskKind::Unknown;
    bool TrueBit = CE->getZExtValue() != 0;

    if (TrueBit && !HasFirstOne) {
      FirstOne = i;
      HasFirstOne = true;
    } else if (!TrueBit && !HasFirstZero) {
      FirstZero = i;
      HasFirstZero = true;
    } else if (TrueBit) {
      // flipping bits on again ->abort
      return BVMaskKind::Unknown;
    }
  }

  return BVMaskKind::Interval;
}

enum class BVKind : int8_t {
  Unknown,   // could not infer pattern
  AllUndef,  // all lanes undef
  Broadcast, // broadcast
  Seq,       // (0, .., 255) Sequence
  SeqBlock,  // (0, .., 15) ^ 16
  BlockSeq,  // 0^16, 1^16, 2^16
};

BVKind AnalyzeMaskView(MaskView &MV, unsigned &FirstDef, unsigned &LastDef,
                       int64_t &Stride, unsigned &BlockLength,
                       unsigned &NumElements) {
  // Check UNDEF or FirstDef
  NumElements = 0;
  bool AllUndef = true;
  FirstDef = 0;
  LastDef = 0;
  SDValue FirstInsertedV;
  for (unsigned i = 0; i < MV.getNumElements(); ++i) {
    auto ES = MV.getSourceElem(i);
    if (!ES.isDefined())
      continue;
    if (!ES.isElemInsert())
      return BVKind::Unknown;
    ++NumElements;

    // mark first non-undef position
    if (AllUndef) {
      FirstInsertedV = ES.V;
      FirstDef = i;
      AllUndef = false;
    }
    LastDef = i;
  }
  if (AllUndef) {
    return BVKind::Unknown;
  }

  // We know at this point that all source elements are scalar insertions or
  // undef

  // Check broadcast
  bool IsBroadcast = true;
  for (unsigned i = FirstDef + 1; i < MV.getNumElements(); ++i) {
    auto ES = MV.getSourceElem(i);
    assert((!ES.isDefined() || ES.isElemInsert()) &&
           "should have quit during first pass");

    bool SameAsFirst = FirstInsertedV == ES.V;
    if (!SameAsFirst && ES.isDefined()) {
      IsBroadcast = false;
    }
  }
  if (IsBroadcast)
    return BVKind::Broadcast;

  ///// Stride pattern detection /////
  // FIXME clean up

  bool hasConstantStride = true;
  bool hasBlockStride = false;
  bool hasBlockStride2 = false;
  bool firstStride = true;
  int64_t lastElemValue;
  BlockLength = 16;

  // Optional<int64_t> InnerStrideOpt;
  // Optional<int64_t> OuterStrideOpt
  // Optional<unsigned> BlockSizeOpt;

  for (unsigned i = 0; i < MV.getNumElements(); ++i) {
    auto ES = MV.getSourceElem(i);
    if (hasBlockStride) {
      if (i % BlockLength == 0)
        Stride = 1;
      else
        Stride = 0;
    }

    if (!ES.isDefined()) {
      if (hasBlockStride2 && i % BlockLength == 0)
        lastElemValue = 0;
      else
        lastElemValue += Stride;
      continue;
    }

    // is this an immediate constant value?
    auto *constNumElem = dyn_cast<ConstantSDNode>(ES.V);
    if (!constNumElem) {
      hasConstantStride = false;
      hasBlockStride = false;
      hasBlockStride2 = false;
      break;
    }

    // read value
    int64_t elemValue = constNumElem->getSExtValue();

    if (i == FirstDef) {
      // FIXME: Currently, this code requies that first value of vseq
      // is zero.  This is possible to enhance like thses instructions:
      //        VSEQ $v0
      //        VBRD $v1, 2
      //        VADD $v0, $v0, $v1
      if (elemValue != 0) {
        hasConstantStride = false;
        hasBlockStride = false;
        hasBlockStride2 = false;
        break;
      }
    } else if (i > FirstDef && firstStride) {
      // first stride
      Stride = (elemValue - lastElemValue) / (i - FirstDef);
      firstStride = false;
    } else if (i > FirstDef) {
      // later stride
      if (hasBlockStride2 && elemValue == 0 && i % BlockLength == 0) {
        lastElemValue = 0;
        continue;
      }
      int64_t thisStride = elemValue - lastElemValue;
      if (thisStride != Stride) {
        hasConstantStride = false;
        if (!hasBlockStride && thisStride == 1 && Stride == 0 &&
            lastElemValue == 0) {
          hasBlockStride = true;
          BlockLength = i;
        } else if (!hasBlockStride2 && elemValue == 0 &&
                   lastElemValue + 1 == i) {
          hasBlockStride2 = true;
          BlockLength = i;
        } else {
          // not blockStride anymore.  e.g. { 0, 1, 2, 3, 0, 0, 0, 0 }
          hasBlockStride = false;
          hasBlockStride2 = false;
          break;
        }
      }
    }

    // track last elem value
    lastElemValue = elemValue;
  }

  if (hasConstantStride)
    return BVKind::Seq;
  if (hasBlockStride) {
    int64_t blockLengthLog = log2(BlockLength);
    if (pow(2, blockLengthLog) == BlockLength)
      return BVKind::BlockSeq;
    return BVKind::Unknown;
  }
  if (hasBlockStride2) {
    int64_t blockLengthLog = log2(BlockLength);
    if (pow(2, blockLengthLog) == BlockLength)
      return BVKind::SeqBlock;
  }
  return BVKind::Unknown;
}

/// MaskShuffleAnalysis {

class ZeroDefaultingView : public MaskView {
protected:
  MaskView &BaseMV;
  SDValue ConstZeroV;
  ElemSelect ZeroInsert;

public:
  ZeroDefaultingView(MaskView &BaseMV, CustomDAG &CDAG)
      : BaseMV(BaseMV), ConstZeroV(CDAG.getConstant(0, MVT::i32)),
        ZeroInsert(ConstZeroV) {}
};

// shows only those elements of a vNi1 vector that are sourced from SX-registers
// or vector registers
class VRegView final : public ZeroDefaultingView {
public:
  VRegView(CustomDAG &CDAG, MaskView &BitMV)
      : ZeroDefaultingView(BitMV, CDAG) {}

  ~VRegView() {}

  // get the element selection at i
  ElemSelect getSourceElem(unsigned DestIdx) override {
    auto ZeroInsert = ElemSelect(ConstZeroV);

    auto ES = BaseMV.getSourceElem(DestIdx);
    // Default
    if (!ES.isDefined())
      return ElemSelect::Undef();

      // insertion from scalar registers (not a constant)
#if 0
    LLVM_DEBUG(dbgs() << "VRegView ES: " << DestIdx << " value: ";
               ES.V->print(dbgs());
               dbgs() << " with value type "
                      << ES.V.getValueType().getEVTString() << "\n";);
#endif

    // Only model element insertions
    if (ES.isElemInsert() && !isa<ConstantSDNode>(ES.V)) {
      return ES;
    }

    // Otw, produce '0' for safe OR-ing of the two views
    return ZeroInsert;
  }

  // the abstract type of this mask
  EVT getValueType() const override {
    return MVT::getVectorVT(MVT::i32,
                            BaseMV.getValueType().getVectorNumElements());
  }

  unsigned getNumElements() const override {
    return getValueType().getVectorNumElements();
  }
};

class BitMaskView final : public ZeroDefaultingView {
public:
  BitMaskView(MaskView &BitMV, CustomDAG &CDAG)
      : ZeroDefaultingView(BitMV, CDAG) {}
  ~BitMaskView() {}

  // get the element selection at i
  ElemSelect getSourceElem(unsigned DestIdx) override {
    auto ES = BaseMV.getSourceElem(DestIdx);

#if 0
    LLVM_DEBUG(dbgs() << "OriginalMV ES: " << DestIdx << " value: ";
               ES.V->print(dbgs());
               dbgs() << " with value type "
                      << ES.V.getValueType().getEVTString() << "\n";);
#endif

    if (!ES.isDefined())
      return ES;

    // insertion from scalar registers
    if (ES.isElemInsert() && !isa<ConstantSDNode>(ES.V)) {
      // produce '0' for safe OR-ing of the two views
      return ZeroInsert;
    }

    // Otw, this is a proper bit transfer
    return ES;
  }

  // the abstracr type of this mask
  EVT getValueType() const override { return BaseMV.getValueType(); }

  unsigned getNumElements() const override {
    return getValueType().getVectorNumElements();
  }
};

static bool hasNonZeroEntry(MaskView &MV) {
  for (unsigned i = 0; i < MV.getNumElements(); ++i) {
    auto ES = MV.getSourceElem(i);
    if (!ES.isDefined())
      continue;
    if (!ES.isElemInsert())
      return true;

    auto ConstV = dyn_cast<ConstantSDNode>(ES.V);
    if (!ConstV || (0 != ConstV->getZExtValue()))
      return true;
  }
  return false;
}

// match a 64 bit segment, mapping out all source bits
// FIXME this implies knowledge about the underlying object structure
MaskShuffleAnalysis::MaskShuffleAnalysis(MaskView &MV, CustomDAG &CDAG)
    : MV(MV) {
  IsConstantOne.reset();
  UndefBits.reset();

  // First, check for any insertions from scalar registers

  // this view only reflects insertions of actual i1 bits (from other mask
  // registers, or MVT::i32 constants)
  BitMaskView BitMV(MV, CDAG);

  const unsigned NumEls = BitMV.getNumElements();
  const unsigned SXRegSize = 64;
  // loop over all sub-registers (sx parts of v256)
  for (unsigned PartIdx = 0; PartIdx * SXRegSize < NumEls; ++PartIdx) {
    const unsigned DestPartBase = PartIdx * SXRegSize;
    const unsigned NumPartBits = std::min(SXRegSize, NumEls - DestPartBase);

    unsigned NumMissingBits = NumPartBits; // keeps track of matcher rouds

    // described all
    ResPart Part(PartIdx);

    while (NumMissingBits > 0) {
      BitSelect Sel;
      for (unsigned i = 0; i < NumPartBits; ++i) {
        ElemSelect ES = BitMV.getSourceElem(i + DestPartBase);
        // skip both kinds of undef (no value transfered or source is undef)
        if (!ES.isDefined()) {
          UndefBits[DestPartBase + i] = true;
          NumMissingBits--;
          continue;
        }

        // inserted bit constants
        if (!ES.isElemTransfer()) {
          NumMissingBits--;
          // This only works because we know that the BitMaskView will mask-out
          // any non-constant bit insertions
          auto ConstBit = cast<ConstantSDNode>(ES.V);
          bool IsTrueBit = 0 != ConstBit->getZExtValue();
          IsConstantOne[i] = IsTrueBit;
          continue;
        }

        // map a new source (and a shift amount)
        unsigned SrcPartIdx =
            (ES.ExtractIdx / SXRegSize); // sx sub-register to chose from
        // required shift amount of the elements of the sub register
        int64_t ShiftAmount = (ES.ExtractIdx % SXRegSize) - i;
        if (!Sel.SrcVal) {
          Sel.SrcVal = ES.V;
          Sel.SrcValPart = SrcPartIdx;
          Sel.ShiftAmount = ShiftAmount;
          Sel.SrcSelMask |= 1 << ES.ExtractIdx;
          NumMissingBits--;
          continue;
        }

        // Copy all bits with similar alignment
        if ((Sel.SrcVal == ES.V && Sel.SrcValPart == SrcPartIdx) &&
            Sel.ShiftAmount == ShiftAmount) {
          Sel.SrcSelMask |= 1 << ES.ExtractIdx;
          NumMissingBits--;
          continue;
        }

        // misaligned bit // TODO start from here next round
      }
      if (Sel.SrcVal) {
        Part.Selects.push_back(Sel);
      }
    }

    LLVM_DEBUG(Part.print(dbgs()););
    Segments.push_back(Part);
  }
}

SDValue MaskShuffleAnalysis::synthesize(SDValue Passthru, BitSelect &BSel,
                                        SDValue SXV, CustomDAG &CDAG) const {
  const uint64_t AllSetMask = (uint64_t)-1;

  // match full register copies
  if ((BSel.SrcSelMask == AllSetMask) && (BSel.ShiftAmount == 0)) {
    return SXV;
  }

  // AND active lanes
  SDValue MaskedV = SXV;
  if (BSel.SrcSelMask != AllSetMask) {
    MaskedV = CDAG.DAG.getNode(
        ISD::AND, CDAG.DL, SXV.getValueType(),
        {MaskedV, CDAG.getConstant(BSel.SrcSelMask, MVT::i64)});
  }

  // shift (trivial 0 case handled internally)
  SDValue ShiftV = CDAG.createElementShift(MaskedV.getValueType(), MaskedV,
                                           BSel.ShiftAmount, SDValue());

  // OR-in passthru
  SDValue ResV = ShiftV;
  if (Passthru) {
    ResV = CDAG.DAG.getNode(ISD::OR, CDAG.DL, SXV.getValueType(),
                            {ShiftV, Passthru});
  }

  return ResV;
}

bool MaskShuffleAnalysis::analyzeVectorSources(bool &AllTrue) const {
  // Check whether all non-scalar sources are the constant `1` or undef
  AllTrue = true;
  for (unsigned i = 0; i < IsConstantOne.size(); ++i) {
    if (!UndefBits[i] && !IsConstantOne[i])
      AllTrue = false;
    break;
  }
  // all `1` background
  if (AllTrue)
    return true;

  // Check whether all segments are empty (eg all non-scalar sources are `0` or
  // undef)
  for (auto &SXPart : Segments) {
    // bit transfer from vector mask reg
    if (!SXPart.empty())
      return false;
    // Non-0 bits in the cconstant background
    for (unsigned SXBit = 0; SXBit < SXRegSize; ++SXBit) {
      unsigned BitPos = SXPart.ResPartIdx * SXRegSize + SXBit;
      if (!UndefBits[BitPos] && IsConstantOne[BitPos]) {
        return false;
      }
    }
  }

  // all `0` background
  AllTrue = false;
  return true;
}

// materialize the code to synthesize this operation
SDValue MaskShuffleAnalysis::synthesize(CustomDAG &CDAG, EVT LegalMaskVT) {
  Packing PackFlag = LegalMaskVT.getVectorNumElements() > 256 ? Packing::Dense
                                                              : Packing::Normal;

  // this view reflects exactly those insertions that are non-constant and have
  // a MVT::i32 type
  VRegView VectorMV(CDAG, MV);
  SDValue BlendV; // VM to be OR-ed into the resulting vector

  // actual element count
  const unsigned NumElems = MV.getNumElements();
  // result type element count
  const unsigned LegalNumElems = LegalMaskVT.getVectorNumElements();
  bool HasScalarSourceEntries = hasNonZeroEntry(VectorMV);

  // There are insertions of scalar register bits.
  // CodeGen those as insertions into 'BlendV' to OR-them in later.
  if (HasScalarSourceEntries) {
    LLVM_DEBUG(dbgs() << ":: has non-trivial insertion in VectorMV ::\n";);
    SDValue AVL = CDAG.getConstEVL(NumElems); // FIXME
    // Synthesize the result vector
    ShuffleAnalysis VSA(VectorMV);
    auto Res = VSA.analyze();
    assert(Res == ShuffleAnalysis::CanSynthesize);
    SDValue VecSourceV =
        VSA.synthesize(CDAG, CDAG.getVectorVT(MVT::i32, LegalNumElems));
    BlendV = CDAG.createMaskCast(VecSourceV, AVL);
  }

  // Check whether this is an all-zero or all-one constant mask (except for scalar register insertions).
  // If not, transfer the XS-sized chunks from their respective source registers.
  SDValue VMAccu;
  bool AllTrue;
  bool HasTrivialBackground = analyzeVectorSources(AllTrue);
  bool AllTrueBackground = HasTrivialBackground && AllTrue;
  bool AllFalseBackground = HasTrivialBackground && !AllTrue;

  if (!HasScalarSourceEntries && AllTrueBackground) {
    // Must not have spurious `1` entries since what is undefined for the
    // vector/constant sources could be the defined insertion of a bit from a
    // scalar register. Short cut when the only occuring constant is a '1'
    VMAccu = CDAG.createUniformConstMask(PackFlag, LegalNumElems, true);

  } else if (AllFalseBackground) {
    // Don't need to check for spurious `1` bits here since
    // the scalar result and the vector/constant results are OR-ed together in
    // the end.
    VMAccu = SDValue(); // Deferring all-false codegen (so we can save on an 'OR' with the blend mask)

  } else {
    // Either non-trivial constant mask or non-trivial incoming bits from other
    // vector masks.
    VMAccu = CDAG.DAG.getUNDEF(LegalMaskVT);

    // There are non-trivial bit transfers from other vector registers
    // Actual mask synthesis code path
    std::map<std::pair<SDValue, unsigned>, SDValue> SourceParts;

    // Extract all source parts
    for (auto &ResPart : Segments) {
      for (auto &BitSel : ResPart.Selects) {
        auto Key =
            std::pair<SDValue, unsigned>(BitSel.SrcVal, BitSel.SrcValPart);
        if (SourceParts.find(Key) != SourceParts.end())
          continue;

        SDValue PartIdxC = CDAG.getConstant(Key.second, MVT::i64);
        auto SXPart = CDAG.createMaskExtract(Key.first, PartIdxC);
        SourceParts[Key] = SXPart;
      }
    }

    // Work through selects, blending and shifting the parts together
    for (auto &ResPart : Segments) {

      // Synthesize the constant background
      unsigned BaseConstant = 0;
      for (unsigned i = 0; i < SXRegSize; ++i) {
        unsigned BitPos = i + ResPart.ResPartIdx * SXRegSize;
        if (IsConstantOne[BitPos])
          BaseConstant |= (1 << i);
      }
      SDValue SXAccu = CDAG.getConstant(BaseConstant, MVT::i64);

      // synthesize all operations that feed into this destionation sx part
      for (auto &BitSel : ResPart.Selects) {
        auto ItExtractedSrc = SourceParts.find(
            std::pair<SDValue, unsigned>(BitSel.SrcVal, BitSel.SrcValPart));
        assert(ItExtractedSrc != SourceParts.end());
        SXAccu = synthesize(SXAccu, BitSel, ItExtractedSrc->second, CDAG);
      }

      // finally, insert the SX part into the the actual VM
      VMAccu = CDAG.createMaskInsert(
          VMAccu, CDAG.getConstant(ResPart.ResPartIdx, MVT::i64), SXAccu);
    }
  }

  // OR-in the BlendV (values inserted from scalar regs)
  if (BlendV && VMAccu) {
    return CDAG.getNode(ISD::OR, LegalMaskVT, {VMAccu, BlendV});
  }
  if (BlendV)
    return BlendV;
  if (VMAccu)
    return VMAccu;
  return CDAG.createUniformConstMask(PackFlag, LegalNumElems, false);
}

/// } MaskShuffleAnalysis

/// ShuffleAnalysis {

/// Scalar Shuffle Strategy {
// This shuffle strategy extracts all source lanes and inserts them into the
// result vector

// extract all vector elements and insert them back at the right positions
struct ScalarTransferOp final : public AbstractShuffleOp {
  LaneBits InsertPositions;

  ScalarTransferOp(LaneBits DefinedLanes) : InsertPositions(DefinedLanes) {}
  virtual ~ScalarTransferOp() {}

  // transfer all insert positions to their destination
  virtual SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) {
    SDValue AccuV = PartialV;

    // TODO caching of extracted element..
    where_true(InsertPositions, [&](unsigned Idx) {
      auto ES = MV.getSourceElem(Idx);
      if (!ES.isDefined())
        return IterContinue;

      // isolate the scalar element
      SDValue SrcElemV;
      if (ES.isElemTransfer()) {
        SrcElemV = CDAG.getVectorExtract(
            ES.V, CDAG.getConstant(ES.getElemIdx(), MVT::i64));
      } else {
        assert(ES.isElemInsert());
        SrcElemV = ES.V;
      }

      // insert it
      AccuV = CDAG.getVectorInsert(AccuV, SrcElemV,
                                   CDAG.getConstant(Idx, MVT::i64));

      return IterContinue;
    });
    return AccuV;
  }

  virtual void print(raw_ostream &out) const { out << "Scalar Transfer"; }
};

struct ScalarTransferStrategy final : public ShuffleStrategy {
  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {
    PartialShuffleState FinalState;

    // provides all missing lanes
    FinalState.MissingLanes.reset();
    CB(new ScalarTransferOp(FromState.MissingLanes), FinalState);
  }
};

/// } Scalar Shuffle Strategy

/// VMV Shuffle Strategy {
// This strategy emits one VMV Op that transfers an entire subvector
// either of the accumulator or the incoming vector.

struct VMVShuffleOp final : public AbstractShuffleOp {
  unsigned DestStartPos;
  unsigned SubVectorLength;
  int32_t ShiftAmount;
  SDValue SrcVector;

  VMVShuffleOp(unsigned DestStartPos, unsigned SubVectorLength,
               int32_t ShiftAmount, SDValue SrcVector)
      : DestStartPos(DestStartPos), SubVectorLength(SubVectorLength),
        ShiftAmount(ShiftAmount), SrcVector(SrcVector) {}

  ~VMVShuffleOp() override {}

  void print(raw_ostream &out) const override {
    out << "VMV { SubVL: " << SubVectorLength
        << ", DestStartPos: " << DestStartPos
        << ", ShiftAmount: " << ShiftAmount << ", Src: ";
    SrcVector->print(out);
    out << " }\n";
  }

  unsigned getAVL() const { return DestStartPos + SubVectorLength; }

  // transfer all insert positions to their destination
  SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) override {
    // noop VMV
    if (ShiftAmount == 0 && PartialV->isUndef())
      return SrcVector;

    LaneBits VMVMask;
    // Synthesize the mask
    VMVMask.reset();
    for (size_t i = 0; i < getAVL(); ++i) {
      VMVMask[i] =
          (i >= (size_t)DestStartPos) && ((i - DestStartPos) < SubVectorLength);
    }
    SDValue MaskV = CDAG.createConstMask(getAVL(), VMVMask);
    SDValue VL = CDAG.getConstEVL(getAVL());
    SDValue ShiftV = CDAG.getConstant(ShiftAmount, MVT::i32);

    SDValue ResV =
        CDAG.createVMV(PartialV.getValueType(), SrcVector, ShiftV, MaskV, VL);
    return CDAG.createSelect(ResV.getValueType(), ResV, PartialV, MaskV, VL);
  }
};

struct VMVShuffleStrategy final : public ShuffleStrategy {
  // greedily match the longest subvector move from \p MV starting at \p
  // SrcStartPos and reading from the source vector \p SrcValue.
  // \returns  the last matching position
  unsigned matchSubvectorMove(MaskView &MV, SDValue SrcValue,
                              unsigned DestStartPos, unsigned SrcStartPos) {
    unsigned LastProperMatch = SrcStartPos;
    for (unsigned i = 1; DestStartPos + i < MV.getNumElements(); ++i) {
      auto ES = MV.getSourceElem(DestStartPos + i);
      // skip over undefined elements
      if (!ES.isDefined())
        continue;
      if (!ES.isElemTransfer())
        continue;

      // check for a contiguous element transfer from the same source vector
      if (ES.V != SrcValue)
        return LastProperMatch;
      unsigned SrcOffset = SrcStartPos + i;
      if (SrcOffset != ES.getElemIdx())
        return LastProperMatch;

      LastProperMatch = SrcOffset;
    }

    return LastProperMatch;
  }

  static int32_t WrapShiftAmount(int32_t ShiftAmount) {
    if (std::abs(ShiftAmount) <= 127)
      return ShiftAmount;
    if (ShiftAmount > 0)
      return -(256 - ShiftAmount);
    return 256 + ShiftAmount;
  }

  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {
    // Seek the largest, lowest shift amount subvector
    SDValue BestSourceV;
    unsigned LongestSVSrcStart = 0;
    unsigned LongestSVDestStart = 0;
    unsigned LongestSubvector = 0;

    // Scan for the largest subvector match
    for (unsigned DestStartIdx = 0;
         DestStartIdx + LongestSubvector < MV.getNumElements();
         ++DestStartIdx) {

      if (!FromState.isMissing(DestStartIdx))
        continue;

      auto ES = MV.getSourceElem(DestStartIdx);
      if (!ES.isDefined())
        continue;
      if (ES.isElemInsert())
        continue;

      SDValue SrcVectorV = ES.V;
      unsigned SrcStartIdx = ES.getElemIdx();
      int32_t ShiftAmount = SrcStartIdx - (int32_t)DestStartIdx;

      // TODO allow wrapping
      unsigned SrcLastMatchIdx =
          matchSubvectorMove(MV, SrcVectorV, DestStartIdx, SrcStartIdx);
      unsigned LastMatchedSVPos = SrcLastMatchIdx - SrcStartIdx;
      unsigned MatchedSVLen = LastMatchedSVPos + 1;

      // new contender
      int32_t BestShiftAmount = LongestSVSrcStart - (int32_t)LongestSVDestStart;
      if ((MatchedSVLen > LongestSubvector) ||
          ((MatchedSVLen == LongestSubvector) &&
           (ShiftAmount < BestShiftAmount))) {
        LongestSVSrcStart = SrcStartIdx;
        LongestSVDestStart = DestStartIdx;
        LongestSubvector = MatchedSVLen;
        BestSourceV = SrcVectorV;
      }
    }

    // TODO cost considerations
    const unsigned MinSubvectorLen = 3;
    if (LongestSubvector < MinSubvectorLen) {
      return;
    }

    // Construct VMV and feed it to the callback
    PartialShuffleState Res = FromState;
    for (unsigned DestIdx = LongestSVDestStart;
         DestIdx < LongestSVDestStart + LongestSubvector; ++DestIdx) {
      Res.unsetMissing(DestIdx);
    }

    int32_t ShiftAmount =
        WrapShiftAmount(LongestSVSrcStart - (int32_t)LongestSVDestStart);
    auto *VMVOp = new VMVShuffleOp(LongestSVDestStart, LongestSubvector,
                                   ShiftAmount, BestSourceV);
    CB(VMVOp, Res);
  }
};

/// } VMV Shuffle Strategy

/// Legacy Pattern Strategy {

struct PatternShuffleOp final : public AbstractShuffleOp {
  BVKind PatternKind;
  unsigned FirstDef;
  unsigned LastDef;
  int64_t Stride;
  unsigned BlockLength;
  unsigned NumElems;

  PatternShuffleOp(BVKind PatternKind, unsigned FirstDef, unsigned LastDef,
                   int64_t Stride, unsigned BlockLength, unsigned NumElems)
      : PatternKind(PatternKind), FirstDef(FirstDef), LastDef(LastDef),
        Stride(Stride), BlockLength(BlockLength), NumElems(NumElems) {}

  ~PatternShuffleOp() override {}

  void print(raw_ostream &out) const override {
    out << "PatternShuffle { FirstDef: " << FirstDef << ", LastDef: " << LastDef
        << " }\n";
  }

  // transfer all insert positions to their destination
  SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) override {
    EVT LegalResVT =
        PartialV.getValueType(); // LegalizeVectorType(Op.getValueType(),
                                 // Op, DAG, Mode);
    bool Packed = IsPackedType(LegalResVT);
    unsigned NativeNumElems = LegalResVT.getVectorNumElements();

    EVT ElemTy = PartialV.getValueType().getVectorElementType();

    // Include the last defined element in the broadcast
    SDValue OpVectorLength =
        CDAG.getConstant(Packed ? (LastDef + 1) / 2 : LastDef + 1, MVT::i32);

    SDValue TrueMask = CDAG.createUniformConstMask(Packing::Normal, NativeNumElems, true);

    switch (PatternKind) {

    // Could not detect pattern
    case BVKind::Unknown:
      llvm_unreachable("Cannot synthesize the 'Unknown' pattern!");

    // Fold undef
    case BVKind::AllUndef: {
      LLVM_DEBUG(dbgs() << "::AllUndef\n");
      return CDAG.getUndef(LegalResVT);
    }

    case BVKind::Broadcast: {
      LLVM_DEBUG(dbgs() << "::Broadcast\n");
      SDValue ScaVal = MV.getSourceElem(FirstDef).V;
      LLVM_DEBUG(ScaVal->dump());
      return CDAG.CreateBroadcast(LegalResVT, ScaVal, OpVectorLength);
    }

    case BVKind::Seq: {
      LLVM_DEBUG(dbgs() << "::Seq\n");
      // detected a proper stride pattern
      SDValue SeqV = CDAG.CreateSeq(LegalResVT, OpVectorLength);
      if (Stride == 1) {
        LLVM_DEBUG(dbgs() << "ConstantStride: VEC_SEQ\n");
        LLVM_DEBUG(CDAG.dumpValue(SeqV));
        return SeqV;
      }

      SDValue StrideV = CDAG.CreateBroadcast(
          LegalResVT, CDAG.getConstant(Stride, ElemTy), OpVectorLength);
      SDValue ret = CDAG.getNode(VEISD::VVP_MUL, LegalResVT,
                                 {SeqV, StrideV, TrueMask, OpVectorLength});
      LLVM_DEBUG(dbgs() << "ConstantStride: VEC_SEQ * VEC_BROADCAST\n");
      LLVM_DEBUG(CDAG.dumpValue(StrideV));
      LLVM_DEBUG(CDAG.dumpValue(ret));
      return ret;
    }

    case BVKind::SeqBlock: {
      LLVM_DEBUG(dbgs() << "::SeqBlock\n");
      // codegen for <0, 1, .., 15, 0, 1, .., ..... > constant patterns
      // constant == VSEQ % blockLength
      SDValue sequence = CDAG.CreateSeq(LegalResVT, OpVectorLength);
      SDValue modulobroadcast = CDAG.CreateBroadcast(
          LegalResVT, CDAG.getConstant(BlockLength - 1, ElemTy),
          OpVectorLength);

      SDValue modulo =
          CDAG.getNode(VEISD::VVP_AND, LegalResVT,
                       {sequence, modulobroadcast, TrueMask, OpVectorLength});

      LLVM_DEBUG(dbgs() << "BlockStride2: VEC_SEQ & VEC_BROADCAST\n");
      LLVM_DEBUG(CDAG.dumpValue(sequence));
      LLVM_DEBUG(CDAG.dumpValue(modulobroadcast));
      LLVM_DEBUG(CDAG.dumpValue(modulo));
      return modulo;
    }

    case BVKind::BlockSeq: {
      LLVM_DEBUG(dbgs() << "::BlockSeq\n");
      // codegen for <0, 0, .., 0, 0, 1, 1, .., 1, 1, .....> constant patterns
      // constant == VSEQ >> log2(blockLength)
      int64_t blockLengthLog = log2(BlockLength);
      SDValue sequence = CDAG.CreateSeq(LegalResVT, OpVectorLength);
      SDValue shiftbroadcast = CDAG.CreateBroadcast(
          LegalResVT, CDAG.getConstant(blockLengthLog, ElemTy), OpVectorLength);

      SDValue shift =
          CDAG.getNode(VEISD::VVP_SRL, LegalResVT,
                       {sequence, shiftbroadcast, TrueMask, OpVectorLength});
      LLVM_DEBUG(dbgs() << "BlockStride: VEC_SEQ >> VEC_BROADCAST\n");
      LLVM_DEBUG(sequence.dump());
      LLVM_DEBUG(shiftbroadcast.dump());
      LLVM_DEBUG(shift.dump());
      return shift;
    }
    }
    llvm_unreachable("UNREACHABLE!");
  }
};

class LegacyPatternStrategy final : public ShuffleStrategy {
public:
  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {
    // Seek the largest, lowest shift amount subvector
    // TODO move this to the planning stage
    unsigned FirstDef = 0;
    unsigned LastDef = 0;
    int64_t Stride = 0;
    unsigned BlockLength = 0;
    unsigned NumElems = 0;

    BVKind PatternKind =
        AnalyzeMaskView(MV, FirstDef, LastDef, Stride, BlockLength, NumElems);

    if (PatternKind == BVKind::Unknown)
      return;

    // This is the number of LSV that may be used to represent a BUILD_VECTOR
    // Otw, this defaults to VLD of a constant
    // FIXME move this to TTI
    const unsigned InsertThreshold = 4;

    // Always use broadcast if you can -> this enables implicit broadcast
    // matching during isel (eg vfadd_vsvl) if one operand is a VEC_BROADCAST
    // node
    // TODO preserve the bitmask in VEC_BROADCAST to expand VEC_BROADCAST late
    // into LVS when its not folded
    if ((PatternKind != BVKind::Broadcast) && (NumElems < InsertThreshold)) {
      return;
    }

    // all missing bits provided
    PartialShuffleState FinalState;
    FinalState.MissingLanes.reset();

    CB(new PatternShuffleOp(PatternKind, FirstDef, LastDef, Stride, BlockLength,
                            NumElems),
       FinalState);
  }
};

/// } Legacy Pattern Strategy

/// Broadcast Strategy {

// This strategy tries to identify the most-frequent element to
// broadcast-and-merge

struct BroadcastOp final : public AbstractShuffleOp {
  ElemSelect SourceElem;
  LaneBits TargetLanes;
  unsigned MaxAVL;

  BroadcastOp(ElemSelect SourceElem, LaneBits TargetLanes, unsigned MaxAVL)
      : SourceElem(SourceElem), TargetLanes(TargetLanes), MaxAVL(MaxAVL) {}

  ~BroadcastOp() {}

  SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) {
    SDValue ScalarSrcV;
    if (SourceElem.isElemInsert()) {
      ScalarSrcV = SourceElem.V;
    } else {
      ScalarSrcV = CDAG.getVectorExtract(SourceElem.V, SourceElem.ExtractIdx);
    }

    EVT VecTy = PartialV.getValueType();
    const unsigned NumElems = VecTy.getVectorNumElements();

    const SDValue PivotV = CDAG.getConstEVL(MaxAVL);
    SDValue BlendMaskV = CDAG.createConstMask(NumElems, TargetLanes);
    SDValue BroadcastV = CDAG.CreateBroadcast(VecTy, ScalarSrcV, PivotV);
    return CDAG.createSelect(VecTy, BroadcastV, PartialV, BlendMaskV, PivotV);
  }

  void print(raw_ostream &out) const {
    out << "Broadcast (AVL: " << MaxAVL << ", Elem: ";
    SourceElem.print(out) << "\n";
  }
};

class BroadcastStrategy final : public ShuffleStrategy {
public:
  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {

    std::unordered_map<ElemSelect, unsigned> ESMap;

    unsigned MaxVL = 0;

    // Create a histogram of all selected values
    FromState.for_missing([&](unsigned Idx) {
      if (!MV.getSourceElem(Idx).isDefined())
        return IterContinue;
      MaxVL = Idx + 1;

      ElemSelect ES = MV.getSourceElem(Idx);
      auto ItES = ESMap.find(ES);
      unsigned Count = 0;
      if (ItES != ESMap.end()) {
        Count = ItES->second + 1;
      }
      ESMap[ES] = Count;
      return IterContinue;
    });

    // find the most frequent (and cheapest) element to broadcast and blend
    unsigned BestCount = 0;
    ElemSelect BestES;
    for (const auto ItElemCount : ESMap) {
      if (ItElemCount.second > BestCount) {
        BestES = ItElemCount.first;
        BestCount = ItElemCount.second;
      }
    }

    // FIXME cost considerations
    const unsigned BroadcastThreshold = 4;
    if (BestCount < BroadcastThreshold)
      return;

    // tick off all merged positions
    LaneBits BroadcastMask;
    BroadcastMask.reset();
    for (unsigned i = 0; i < MV.getNumElements(); ++i) {
      auto ES = MV.getSourceElem(i);
      if (!ES.isDefined())
        continue;
      if (BestES != ES)
        continue;
      FromState.unsetMissing(i);
      BroadcastMask[i] = true;
    }

    CB(new BroadcastOp(BestES, BroadcastMask, MaxVL), FromState);
  }
};

/// } Broadcast Strategy

/// Constant Elements {
// This strategy emits a constant vector with all the elements and loads it from
// memory.
struct ConstantElemOp final : public AbstractShuffleOp {
  Constant *VecConstant;
  ConstantElemOp(Constant *VecConstant) : VecConstant(VecConstant) {}
  ~ConstantElemOp() {}

  SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) {
    EVT LegalResVT = PartialV.getValueType();
    const EVT PtrVT = MVT::i64;
    // const unsigned LegalNumElems = LegalResVT.getVectorNumElements();
    Align VecAlignBytes(8);
    SDValue ConstantPtrV =
        CDAG.DAG.getConstantPool(VecConstant, PtrVT, VecAlignBytes);
    SDValue ResultV;
    CDAG.weaveIntoRootChain([&]() {
      SDValue Chain = CDAG.DAG.getEntryNode();
#if 1
      // FIXME only works for 32/64bit elements
      const unsigned NumBufferElems =
          GetVectorNumElements(VecConstant->getType());
      SDValue MaskV = CDAG.createUniformConstMask(
          Packing::Normal, LegalResVT.getVectorNumElements(), true);
      SDValue VLV = CDAG.getConstEVL(NumBufferElems);
      ResultV = CDAG.getVVPLoad(LegalResVT, Chain, ConstantPtrV, MaskV, VLV);
#else
      MachinePointerInfo MPI;
      ResultV = CDAG.DAG.getLoad(LegalResVT, CDAG.DL, Chain, ConstantPtrV, MPI);
#endif
      return SDValue(ResultV.getNode(), 1);
    });
    return ResultV;
  }

  void print(raw_ostream &out) const {
    out << "ConstantElemShuffle (";
    VecConstant->print(out);
    out << ")\n";
  }
};

class ConstantElemStrategy final : public ShuffleStrategy {
public:
  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {

    Type *ElemTy = nullptr;
    unsigned NumConstElems = 0;
    std::vector<Constant *> CVec;
    unsigned LastConstElemVL = 0;
    for (unsigned Idx = 0; Idx < MV.getNumElements(); ++Idx) {
      auto ES = MV.getSourceElem(Idx);
      Constant *ElemC = nullptr;
      if (!ES.isDefined()) {
        ElemC = nullptr;
      } else if (!ES.isElemInsert()) {
        ElemC = nullptr;
      } else if (ConstantSDNode *ElemN = dyn_cast<ConstantSDNode>(ES.V)) {
        ElemC = const_cast<ConstantInt *>(ElemN->getConstantIntValue());
      } else if (ConstantFPSDNode *ElemN = dyn_cast<ConstantFPSDNode>(ES.V)) {
        ElemC = const_cast<ConstantFP *>(ElemN->getConstantFPValue());
      }

      if (ElemC) {
        if (!ElemTy)
          ElemTy = ElemC->getType();
        LastConstElemVL = Idx + 1;
        ++NumConstElems;
      }

      CVec.push_back(ElemC);
    }

    // FIXME Heuristics
    const unsigned ConstElemThreshold = 8;
    if (NumConstElems < ConstElemThreshold)
      return;

    // Truncate up to the last constant entry
    CVec.resize(LastConstElemVL);

    auto UDVal = UndefValue::get(ElemTy);
    for (unsigned Idx = 0; Idx < CVec.size(); ++Idx) {
      if (!CVec[Idx]) {
        CVec[Idx] = UDVal;
      } else {
        FromState.unsetMissing(Idx);
      }
    }

    auto *CV = ConstantVector::get(CVec);
    CB(new ConstantElemOp(CV), FromState);
  }
};

/// } Constant Elements

/// Gather Strategy {

// This strategy provides missing lanes by writing the source register to the
// stack a stack lot and gathering from it to the right lanes (using passthru)

struct GatherShuffleOp final : public AbstractShuffleOp {
  SDValue SrcVectorV;
  std::vector<unsigned> SrcLanes;
  LaneBits TargetLanes;
  unsigned MaxVL;

  GatherShuffleOp(SDValue SrcVectorV, LaneBits TargetLanes, unsigned MaxVL)
      : SrcVectorV(SrcVectorV), TargetLanes(TargetLanes), MaxVL(MaxVL) {}

  ~GatherShuffleOp() {}

  SDValue synthesize(MaskView &MV, CustomDAG &CDAG, SDValue PartialV) {
    // Spill the requires elements of \p SrcVectorV to the stack
    EVT LegalizedSrcVT =
        CDAG.legalizeVectorType(SrcVectorV, VVPExpansionMode::ToNextWidth);

    EVT LegalResVT = PartialV.getValueType();
    const unsigned LegalNumElems = LegalResVT.getVectorNumElements();
    const unsigned ElemBytes =
        LegalResVT.getVectorElementType().getStoreSizeInBits();

    // ptr offset type
    MVT PtrVT = MVT::i64;
    EVT PtrVecVT = CDAG.getVectorVT(PtrVT, LegalNumElems);

    const unsigned SpillAlign = 8;
    //
    // TODO use the smallest possible spill type
#if 1
    auto VecSlotPtr = CDAG.DAG.CreateStackTemporary(LegalizedSrcVT, SpillAlign);
#else
    // FIXME use something like the below code:
    uint64_t TySize = CDAG.getDataLayout().getTypeAllocSize(LegalizeSrcVT);
    int SSFI = MF.getFrameInfo().CreateStackObject(TySize, Align, false);
    SDValue StackSlot = DAG.getFrameIndex(SSFI, TLI.getFrameIndexTy(DL));
    Chain = DAG.getTruncStore(Chain, Location, OpInfo.CallOperand, StackSlot,
                              MachinePointerInfo::getFixedStack(MF, SSFI),
                              TLI.getMemValueType(DL, Ty));
#endif

    // Spill to VecSlot
    MachinePointerInfo MPI;
    SDValue Chain = CDAG.DAG.getStore(CDAG.DAG.getEntryNode(), CDAG.DL,
                                      SrcVectorV, VecSlotPtr, MPI);

    // Compute gahter indices
    SDValue TrueMaskV =
        CDAG.createUniformConstMask(Packing::Normal, LegalNumElems, true);
    SDValue MaskV = PartialV.isUndef()
                        ? TrueMaskV
                        : CDAG.createConstMask(MaxVL, TargetLanes);

    std::vector<SDValue> GatherOffsets;
    for (unsigned Idx = 0; Idx < MaxVL; ++Idx) {
      auto ES = MV.getSourceElem(Idx);
      if (ES.V != SrcVectorV) {
        // Need to emit an inbounds offset here to do the gather without
        // masking.
        GatherOffsets.push_back(CDAG.getConstant(0, PtrVT));
        continue;
      }

      assert(ES.isElemTransfer());
      GatherOffsets.push_back(
          CDAG.getConstant(ElemBytes * ES.getElemIdx(), PtrVT)); // TODO
    }
    for (unsigned Idx = MaxVL; Idx < LegalNumElems; ++Idx) {
      GatherOffsets.push_back(CDAG.getUndef(PtrVT));
    }

    SDValue MaxVLV = CDAG.getConstEVL(MaxVL);
    SDValue BasePtrV = CDAG.CreateBroadcast(PtrVecVT, VecSlotPtr);
    SDValue OffsetV = CDAG.getNode(
        ISD::BUILD_VECTOR, PtrVecVT,
        GatherOffsets); // TODO directly call into constant vector generation
    SDValue GatherPtrV = CDAG.getNode(ISD::ADD, PtrVecVT, {BasePtrV, OffsetV});

    SDValue ElemV =
        CDAG.getVVPGather(LegalResVT, Chain, GatherPtrV, TrueMaskV, MaxVLV);
    Chain = SDValue(ElemV.getNode(), 1);

    // weave in with the root chain
    CDAG.DAG.setRoot(CDAG.getTokenFactor({CDAG.getRootOrEntryChain(), Chain}));

    if (PartialV.isUndef()) {
      return ElemV;
    }
    return CDAG.createSelect(LegalResVT, ElemV, PartialV, MaskV, MaxVLV);
  }

  void print(raw_ostream &out) const {
    out << "GatherShuffle (VL: " << MaxVL << ", SourceVector: ";
    SrcVectorV->print(out);
    out << ")\n";
  }
};

class GatherStrategy final : public ShuffleStrategy {
public:
  void planPartialShuffle(MaskView &MV, PartialShuffleState FromState,
                          PartialShuffleCB CB) override {

    std::unordered_map<SDNode *, unsigned> VecSourceHisto;

    // most frequent vector source
    SDValue MaxV;
    unsigned MaxCount = 0;
    unsigned MaxVL = 0;

    // Find the most frequent vector source
    FromState.for_missing([&](unsigned Idx) {
      ElemSelect ES = MV.getSourceElem(Idx);
      if (!ES.isDefined())
        return IterContinue;
      if (ES.isElemInsert())
        return IterContinue;

      SDNode *SrcN = ES.V.getNode();
      auto ItSrc = VecSourceHisto.find(SrcN);
      unsigned Count = 0;
      if (ItSrc != VecSourceHisto.end()) {
        Count = ItSrc->second + 1;
      }
      if (Count > MaxCount) {
        MaxCount = Count;
        MaxV = ES.V;
        MaxVL = Idx + 1;
      }
      VecSourceHisto[SrcN] = Count;
      return IterContinue;
    });

    // Abort if below a certain threshold
    const unsigned MinGatherElements = 32;
    if (MaxCount < MinGatherElements)
      return;

    // Find all lanes with this source
    LaneBits TargetBits;
    TargetBits.reset();
    FromState.for_missing([&](unsigned Idx) {
      auto ES = MV.getSourceElem(Idx);
      if (!ES.isDefined())
        return IterContinue;
      if (ES.V == MaxV) {
        TargetBits[Idx] = ES.V == MaxV;
        FromState.unsetMissing(Idx);
      }
      return IterContinue;
    });

    CB(new GatherShuffleOp(MaxV, TargetBits, MaxVL), FromState);
  }
};

/// } Gather Strategy

IterControl ShuffleAnalysis::runStrategy(ShuffleStrategy &Strat,
                                         unsigned NumRounds, MaskView &MV,
                                         PartialShuffleState &PSS) {
  for (unsigned i = 0; !PSS.isComplete() && i < NumRounds; ++i) {
    bool Progress = false;
    Strat.planPartialShuffle(
        MV, PSS,
        [&](AbstractShuffleOp *PartialOp, PartialShuffleState NextPSS) {
          Progress = true;
          PSS = NextPSS,
          ShuffleSeq.push_back(std::unique_ptr<AbstractShuffleOp>(PartialOp));
          return IterBreak;
        });
    if (!Progress)
      break;
  }
  return PSS.isComplete() ? IterBreak : IterContinue;
}

ShuffleAnalysis::AnalyzeResult ShuffleAnalysis::analyze() {
  PartialShuffleState PSS = PartialShuffleState::fromInitialMask(MV);

  // Detect simple broadcast, SEQ patterns (that explains the *entire* pattern)
  if (IterBreak == run<LegacyPatternStrategy>(1, MV, PSS))
    return CanSynthesize;
  // Load all constant entries from the constant pool.
  if (IterBreak == run<ConstantElemStrategy>(1, MV, PSS))
    return CanSynthesize;
  // Broadcast and blend the most frequent single element.
  if (IterBreak == run<BroadcastStrategy>(3, MV, PSS))
    return CanSynthesize;
  // Provide elements by VMV.
  if (IterBreak == run<VMVShuffleStrategy>(5, MV, PSS))
    return CanSynthesize;
  // Finally, store vector sources to memory and gather them
  if (IterBreak == run<GatherStrategy>(3, MV, PSS))
    return CanSynthesize;

  // Fallback: LVS-LSV the elements into place.
  ScalarTransferStrategy STS;
  STS.planPartialShuffle(
      MV, PSS, [&](AbstractShuffleOp *PartialOp, PartialShuffleState NextPSS) {
        assert(NextPSS.isComplete() && "scalar transfer is always complete..");
        ShuffleSeq.push_back(std::unique_ptr<AbstractShuffleOp>(PartialOp));
        return IterBreak;
      });

  return CanSynthesize;
}

raw_ostream &ShuffleAnalysis::print(raw_ostream &out) const {
  out << "ShuffleAnalysis. Sequence {\n";
  for (const auto &ShuffleOp : ShuffleSeq) {
    out << "- ";
    ShuffleOp->print(out);
  }
  out << "}\n";
  return out;
}

SDValue ShuffleAnalysis::synthesize(CustomDAG &CDAG, EVT LegalResultVT) {
  LLVM_DEBUG(dbgs() << "Synthesized shuffle sequence:\n"; print(dbgs()));

  SDValue AccuV = CDAG.getUndef(LegalResultVT);
  for (auto &ShuffOp : ShuffleSeq) {
    AccuV = ShuffOp->synthesize(MV, CDAG, AccuV);
  }
  return AccuV;
}

/// } ShuffleAnalysis

} // namespace llvm
