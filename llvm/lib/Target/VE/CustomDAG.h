//===-- CustomDAG.h - VE Custom DAG Nodes ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that VE uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_VE_CUSTOMDAG_H
#define LLVM_LIB_TARGET_VE_CUSTOMDAG_H

#include "VE.h"
#include "VEISelLowering.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLowering.h"
#include <bitset>

namespace llvm {

const unsigned SXRegSize = 64;

/// Helpers {
template <typename ElemT> ElemT &ref_to(std::unique_ptr<ElemT> &UP) {
  return *(UP.get());
}
/// } Helpers

class VESubtarget;

using PosOpt = Optional<unsigned>;

/// } Broadcast, Shuffle, Mask Analysis

//// VVP Machinery {
// VVP property queries
PosOpt GetVVPOpcode(unsigned OpCode);

bool SupportsPackedMode(unsigned Opcode);

bool IsVVPOrVEC(unsigned Opcode);
bool IsVVP(unsigned Opcode);

// Choses the widest element type
EVT getFPConvType(SDNode *Op);

Optional<EVT> getIdiomaticType(SDNode *Op);

VecLenOpt MinVectorLength(VecLenOpt A, VecLenOpt B);

// Whether direct codegen for this type will result in a packed operation
// (requiring a packed VL param..)
bool IsPackedType(EVT SomeVT);

// legalize packed-mode broadcasts into lane replication + broadcast
SDValue LegalizeBroadcast(SDValue Op, SelectionDAG &DAG);

SDValue LegalizeVecOperand(SDValue Op, SelectionDAG &DAG);

// whether this VVP operation has no mask argument
bool HasDeadMask(unsigned VVPOC);

//// } VVP Machinery

Optional<unsigned> getVVPReductionStartParamPos(unsigned ISD);

/// Packing {
using LaneBits = std::bitset<256>;

struct PackedLaneBits {
  LaneBits Bits[2];

  PackedLaneBits() {}

  PackedLaneBits(LaneBits &Lo, LaneBits &Hi) {
    Bits[0] = Lo;
    Bits[1] = Hi;
  }

  void reset() {
    Bits[0].reset();
    Bits[1].reset();
  }

  void flip() {
    Bits[0].flip();
    Bits[1].flip();
  }
  LaneBits &low() { return Bits[0]; }
  LaneBits &high() { return Bits[1]; }

  LaneBits::reference operator[](size_t pos) { return Bits[pos % 2][pos / 2]; }
  bool operator[](size_t pos) const { return Bits[pos % 2][pos / 2]; }
  size_t size() const { return 512; }
};

enum class Packing {
  Normal = 0, // 32/64bits per 64bit elements
  Dense = 1,  // packed mode
};

// Packed interpretation sub element
enum class PackElem : int8_t {
  Lo = 0, // integer (63, 32]
  Hi = 1  // float   (32,  0]
};

static inline MVT getMaskVT(Packing P) {
  return P == Packing::Normal ? MVT::v256i1 : MVT::v512i1;
}

static inline Packing getPackingForVT(EVT VT) {
  assert(VT.isVector());
  return IsPackedType(VT) ? Packing::Dense : Packing::Normal;
}

template <typename MaskBits> Packing getPackingForMaskBits(const MaskBits MB);

/// } Packing

Optional<unsigned> getReductionStartParamPos(unsigned ISD);

Optional<unsigned> getReductionVectorParamPos(unsigned ISD);

Optional<unsigned> PeekForNarrow(SDValue Op);

Optional<SDValue> EVLToVal(VecLenOpt Opt, SDLoc &DL, SelectionDAG &DAG);

bool IsMaskType(EVT Ty);
unsigned GetMaskBits(EVT Ty);

// select an appropriate %evl argument for this element count.
// This will return the correct result for packed mode oeprations (half).
unsigned SelectBoundedVectorLength(unsigned StaticNumElems);

/// Helper class for short hand custom node creation ///
struct CustomDAG {
  const VELoweringInfo &VLI;
  SelectionDAG &DAG;
  SDLoc DL;

  CustomDAG(const VELoweringInfo &VLI, SelectionDAG &DAG, SDLoc DL)
      : VLI(VLI), DAG(DAG), DL(DL) {}

  CustomDAG(const VELoweringInfo &VLI, SelectionDAG &DAG, SDValue WhereOp)
      : VLI(VLI), DAG(DAG), DL(WhereOp) {}

  CustomDAG(const VELoweringInfo &VLI, SelectionDAG &DAG, SDNode *WhereN)
      : VLI(VLI), DAG(DAG), DL(WhereN) {}

  SDValue CreateSeq(EVT ResTy, Optional<SDValue> OpVectorLength) const;

  // create a vector element or scalar bitshift depending on the element type
  // \p ResVT will only be used in case any new node is created
  // dst[i] = src[i + Offset]
  SDValue createElementShift(EVT ResVT, SDValue Src, int Offset,
                             SDValue AVL) const;
  SDValue createScalarShift(EVT ResVT, SDValue Src, int Offset) const;

  SDValue createVMV(EVT ResVT, SDValue SrcV, SDValue OffsetV, SDValue Mask,
                    SDValue Avl) const;
  SDValue createPassthruVMV(EVT ResVT, SDValue SrcV, SDValue OffsetV,
                            SDValue Mask, SDValue PassthruV, SDValue Avl) const;

  SDValue getTargetExtractSubreg(MVT SubRegVT, int SubRegIdx,
                                 SDValue RegV) const;

  /// Packed Mode Support {
  SDValue CreateUnpack(EVT DestVT, SDValue Vec, PackElem E, SDValue AVL);

  SDValue CreatePack(EVT DestVT, SDValue LowV, SDValue HighV, SDValue AVL);

  SDValue CreateSwap(EVT DestVT, SDValue V, SDValue AVL);
  /// } Packed Mode Support

  /// Mask Insert/Extract {
  SDValue CreateExtractMask(SDValue MaskV, SDValue IndexV) const;
  SDValue CreateInsertMask(SDValue MaskV, SDValue ElemV, SDValue IndexV) const;
  /// } Mask Insert/Extract

  SDValue CreateBroadcast(EVT ResTy, SDValue S,
                          Optional<SDValue> OpVectorLength = None) const;

  // Extract an SX register from a mask
  SDValue createMaskExtract(SDValue MaskV, SDValue Idx) const;

  // Extract an SX register from a mask
  SDValue createMaskInsert(SDValue MaskV, SDValue Idx, SDValue ElemV) const;

  // all-true/false mask
  SDValue createUniformConstMask(Packing Packing, unsigned NumElements,
                                 bool IsTrue) const;
  SDValue createUniformConstMask(EVT MaskVT, bool IsTrue) const {
    Packing Packing =
        MaskVT.getVectorNumElements() < 256 ? Packing::Dense : Packing::Normal;
    return createUniformConstMask(Packing, MaskVT.getVectorNumElements(),
                                  IsTrue);
  }
  // materialize a constant mask vector given by \p TrueBits
  template <typename MaskBitsType>
  SDValue createConstMask(unsigned NumElems,
                          const MaskBitsType &TrueBits) const;

  template <typename MaskBitsType>
  SDValue createConstMask(EVT MaskVT, const MaskBitsType &TrueBits) const {
    return createConstMask(MaskVT.getVectorNumElements(), TrueBits);
  }

  // OnTrueV[l] if l < PivotV && Mask[l] else OnFalseV[l]
  SDValue createSelect(EVT ResVT, SDValue OnTrueV, SDValue OnFalseV,
                       SDValue MaskV, SDValue PivotV) const;

  /// getNode {
  SDValue getNode(unsigned OC, SDVTList VTL, ArrayRef<SDValue> OpV) const {
    return DAG.getNode(OC, DL, VTL, OpV);
  }

  SDValue getNode(unsigned OC, ArrayRef<EVT> ResVT,
                  ArrayRef<SDValue> OpV) const {
    return DAG.getNode(OC, DL, ResVT, OpV);
  }

  SDValue getNode(unsigned OC, EVT ResVT, ArrayRef<SDValue> OpV) const {
    return DAG.getNode(OC, DL, ResVT, OpV);
  }
  /// } getNode

  SDValue getVectorExtract(SDValue VecV, unsigned Idx) const {
    return getVectorExtract(VecV, getConstant(Idx, MVT::i32));
  }
  SDValue getVectorExtract(SDValue VecV, SDValue IdxV) const;
  SDValue getVectorInsert(SDValue DestVecV, SDValue ElemV, unsigned Idx) const {
    return getVectorInsert(DestVecV, ElemV, getConstant(Idx, MVT::i32));
  }
  SDValue getVectorInsert(SDValue DestVecV, SDValue ElemV, SDValue IdxV) const;

  SDValue widenOrNarrow(EVT DestVT, SDValue Op) {
    EVT OpVT = Op.getValueType();
    if (OpVT == DestVT)
      return Op;

    if (!OpVT.isVector())
      return Op;

    return createNarrow(DestVT, Op, OpVT.getVectorNumElements());
  }

  SDValue createNarrow(EVT ResTy, SDValue SrcV, uint64_t NarrowLen) {
    return DAG.getNode(VEISD::VEC_NARROW, DL, ResTy,
                       {SrcV, getConstant(NarrowLen, MVT::i32)});
  }

  EVT getVectorVT(EVT ElemVT, unsigned NumElems) const {
    return EVT::getVectorVT(*DAG.getContext(), ElemVT, NumElems);
  }
  inline SDValue getConstEVL(uint32_t EVL) const {
    return getConstant(EVL, MVT::i32);
  }

  SDValue getConstant(uint64_t Val, EVT VT, bool IsTarget = false,
                      bool IsOpaque = false) const;

  SDValue getUndef(EVT VT) const { return DAG.getUNDEF(VT); }

  SDValue getMergeValues(ArrayRef<SDValue> Values) const {
    return DAG.getMergeValues(Values, DL);
  }

  SDValue createNot(SDValue Op, EVT ResVT) const {
    return DAG.getNOT(DL, Op, ResVT);
  }

  EVT getMaskVTFor(SDValue VectorV) const {
    return getVectorVT(MVT::i1, VectorV.getValueType().getVectorNumElements());
  }

  // create a VEC_TOMASK node if VectorV is not a mask already
  SDValue createMaskCast(SDValue VectorV, SDValue AVL) const;

  SDValue getSetCC(SDValue LHS, EVT VT, SDValue RHS, ISD::CondCode CC) const {
    return DAG.getSetCC(DL, VT, LHS, RHS, CC);
  }

  void dumpValue(SDValue V) const;

  SDValue getRootOrEntryChain() const {
    SDValue RootChain = DAG.getRoot();
    if (!RootChain)
      return DAG.getEntryNode();
    return RootChain;
  }

  // weave in a chain into the current root
  void weaveIntoRootChain(std::function<SDValue()> Func) const {
    SDValue OutChain = Func();
    assert(OutChain.getValueType() == MVT::Other); // not a chain!
    DAG.setRoot(getTokenFactor({getRootOrEntryChain(), OutChain}));
  }

  SDValue getTokenFactor(ArrayRef<SDValue> Tokens) const;

  const DataLayout &getDataLayout() const { return DAG.getDataLayout(); }

  // Return a legal vector type for \p Op
  EVT legalizeVectorType(SDValue Op, VVPExpansionMode) const;

  /// VVP {
  SDValue getVVPLoad(EVT LegalResVT, SDValue Chain, SDValue PtrV, SDValue MaskV,
                     SDValue AVL) const;

  SDValue getVVPGather(EVT LegalResVT, SDValue Chain, SDValue PtrVecV,
                       SDValue MaskV, SDValue AVL) const;
  /// } VVP

  LLVMContext &getContext() { return *DAG.getContext(); }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_VE_CUSTOMDAG_H
