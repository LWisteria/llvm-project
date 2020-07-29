//===-- VEISelLowering.h - VE DAG Lowering Interface ------------*- C++ -*-===//
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

#ifndef LLVM_LIB_TARGET_VE_VEISELLOWERING_H
#define LLVM_LIB_TARGET_VE_VEISELLOWERING_H

#include "VE.h"
#include "VELoweringInfo.h"
#include "llvm/CodeGen/TargetLowering.h"

namespace llvm {
class VESubtarget;
struct CustomDAG;

namespace VEISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,

  EQV,         // Equivalence between two integer values.
  XOR,         // Exclusive-or between two integer values.
  CMPI,        // Compare between two signed integer values.
  CMPU,        // Compare between two unsigned integer values.
  CMPF,        // Compare between two floating-point values.
  CMPQ,        // Compare between two quad floating-point values.
  CMOV,        // Select between two values using the result of comparison.

  EH_SJLJ_SETJMP,         // SjLj exception handling setjmp.
  EH_SJLJ_LONGJMP,        // SjLj exception handling longjmp.
  EH_SJLJ_SETUP_DISPATCH, // SjLj exception handling setup_dispatch.

  Hi,
  Lo, // Hi/Lo operations, typically on a global address.

  GETFUNPLT,   // load function address through %plt insturction
  GETTLSADDR,  // load address for TLS access
  GETSTACKTOP, // retrieve address of stack top (first address of
               // locals and temporaries)

  MEMBARRIER, // Compiler barrier only; generate a no-op.

  CALL,            // A call instruction.
  RET_FLAG,        // Return with a flag operand.
  GLOBAL_BASE_REG, // Global base reg for PIC.
  FLUSHW,          // FLUSH register windows to stack.

  // Mask support
  VM_EXTRACT, // VM_EXTRACT(v256i1:mask, i32:i) Extract a SX register from a
              // mask register
  VM_INSERT, // VM_INSERT(v256i1:mask, i32:i, i64:val) Insert a SX register into
             // a mask register
  VM_FIRST = VM_EXTRACT,
  VM_LAST = VM_INSERT,

  /// VEC_ {
  // Packed mode support
  VEC_UNPACK_LO, // unpack the lo (v256i32) slice of a packed v512.32
  VEC_UNPACK_HI, // unpack the hi (v256f32) slice of a packed v512.32
  VEC_PACK,      // pack a lo and a hi vector backinto one v512.32 vector
  VEC_SWAP, // exchange the odd-even positions (v256i32 <> v256f32) or (v512x32
            // <> v512y32) x != y

  // Create a mask that is true where the vector lane is != 0
  VEC_TOMASK, // 0: Vector value, 1: AVL (no mask)
  // Broadcast an SX register
  VEC_BROADCAST, // 0: the value, 1: the vector length (no mask)
  // Create a sequence vector
  VEC_SEQ, // 1: the vector length (no mask)
  VEC_VMV, // custom lowering for vp_vshift

  //// Horizontal operations
  VEC_REDUCE_ANY,
  VEC_POPCOUNT,

  // narrowing marker
  VEC_NARROW, // (Op, vector length)

  // VEC_* operator range
  VEC_FIRST = VEC_UNPACK_LO,
  VEC_LAST = VEC_NARROW,
  /// } VEC_

  // Replication on lower/upper32 bit to other half -> I64
  REPL_F32,
  REPL_I32,

// Internal VVP nodes
#define ADD_VVP_OP(VVP_NAME) VVP_NAME,
#include "VVPNodes.inc"

  /// A wrapper node for TargetConstantPool, TargetJumpTable,
  /// TargetExternalSymbol, TargetGlobalAddress, TargetGlobalTLSAddress,
  /// MCSymbol and TargetBlockAddress.
  Wrapper,
};
} // namespace VEISD

using VecLenOpt = Optional<unsigned>;

struct VVPWideningInfo {
  EVT ResultVT;
  unsigned ActiveVectorLength;
  bool PackedMode;
  bool NeedsPackedMasking;

  bool isValid() const {
    return ActiveVectorLength != 0;
  }

  VVPWideningInfo(EVT ResultVT, unsigned StaticVL, bool PackedMode,
                  bool NeedsPackedMasking)
      : ResultVT(ResultVT), ActiveVectorLength(StaticVL),
        PackedMode(PackedMode), NeedsPackedMasking(NeedsPackedMasking) {}

  VVPWideningInfo()
      : ResultVT(), ActiveVectorLength(0), PackedMode(false),
        NeedsPackedMasking(false) {}
};

class VETargetLowering final : public TargetLowering, public VELoweringInfo {
  const VESubtarget *Subtarget;

  void initRegisterClasses();

  // setOperationAction for all scalar ops
  void initSPUActions();
  // setOperationAction for all vector ops
  void initVPUActions();

public:
  VETargetLowering(const TargetMachine &TM, const VESubtarget &STI);

  TargetLoweringBase::LegalizeAction
  getCustomOperationAction(SDNode &) const override;
  TargetLowering::LegalizeAction
  getActionForExtendedType(unsigned Op, EVT VT) const override;

  /// computeKnownBitsForTargetNode - Determine which of the bits specified
  /// in Mask are known to be either zero or one and return them in the
  /// KnownZero/KnownOne bitsets.
  void computeKnownBitsForTargetNode(const SDValue Op, KnownBits &Known,
                                     const APInt &DemandedElts,
                                     const SelectionDAG &DAG,
                                     unsigned Depth = 0) const override;

  MachineBasicBlock *
  EmitInstrWithCustomInserter(MachineInstr &MI,
                              MachineBasicBlock *MBB) const override;


  const char *getTargetNodeName(unsigned Opcode) const override;
  MVT getScalarShiftAmountTy(const DataLayout &, EVT) const override {
    return MVT::i32;
  }

  SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

  /// Return true if the target has native support for
  /// the specified value type and it is 'desirable' to use the type for the
  /// given node type. e.g. On VE i32 is legal, but undesirable i32 for
  /// AND/OR/XOR instructions since VE doesn't have those instructions for
  /// i32.
  bool isTypeDesirableForOp(unsigned Opc, EVT VT) const override;

  SDValue combineExtBoolTrunc(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineTRUNCATE(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineSetCC(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineSelectCC(SDNode *N, DAGCombinerInfo &DCI) const;
  SDValue combineSelect(SDNode *N, DAGCombinerInfo &DCI) const;

  /// This function looks at SETCC that compares integers. It replaces
  /// SETCC with integer arithmetic operations when there is a legal way
  /// of doing it.
  SDValue optimizeSetCC(SDNode *N, DAGCombinerInfo &DCI) const;

  SDValue generateEquivalentSub(SDNode *N, bool Signed, bool Complement,
                                bool Swap, SelectionDAG &DAG) const;
  SDValue generateEquivalentCmp(SDNode *N, bool UseCompAsBase,
                                SelectionDAG &DAG) const;
  SDValue generateEquivalentLdz(SDNode *N, bool Complement,
                                SelectionDAG &DAG) const;

  // inline asm
  ConstraintType getConstraintType(StringRef Constraint) const override;
  ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                 const char *constraint) const override;
  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

  unsigned getInlineAsmMemConstraint(StringRef ConstraintCode) const override {
    if (ConstraintCode == "o")
      return InlineAsm::Constraint_o;
    return TargetLowering::getInlineAsmMemConstraint(ConstraintCode);
  }

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               StringRef Constraint, MVT VT) const override;

  // scalar ops
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  Register getRegisterByName(const char *RegName, LLT VT,
                             const MachineFunction &MF) const override;

  /// Override to support customized stack guard loading.
  bool useLoadStackGuardNode() const override;
  void insertSSPDeclarations(Module &M) const override;

  /// getSetCCResultType - Return the ISD::SETCC ValueType
  EVT getSetCCResultType(const DataLayout &DL, LLVMContext &Context,
                         EVT VT) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerFormalArguments_64(SDValue Chain, CallingConv::ID CallConv,
                                  bool isVarArg,
                                  const SmallVectorImpl<ISD::InputArg> &Ins,
                                  const SDLoc &dl, SelectionDAG &DAG,
                                  SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;

  bool CanLowerReturn(CallingConv::ID CallConv, MachineFunction &MF,
                      bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &ArgsFlags,
                      LLVMContext &Context) const override;
  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &dl,
                      SelectionDAG &DAG) const override;
  SDValue withTargetFlags(SDValue Op, unsigned TF, SelectionDAG &DAG) const;
  SDValue makeHiLoPair(SDValue Op, unsigned HiTF, unsigned LoTF,
                       SelectionDAG &DAG) const;
  SDValue makeAddress(SDValue Op, SelectionDAG &DAG) const;

  /// VELoweringInfo {
  EVT LegalizeVectorType(EVT ResTy, SDValue Op, SelectionDAG &DAG,
                         VVPExpansionMode) const override;
  /// } VELoweringInfo

  // Widening configuration & legalizer
  VVPWideningInfo pickResultType(CustomDAG &CDAG, SDValue Op,
                                 VVPExpansionMode Mode) const;
  /// Custom Lower {
  // legalize the result vector type for operation \p Op

  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;
  void LowerOperationWrapper(
      SDNode *N, SmallVectorImpl<SDValue> &Results, SelectionDAG &DAG,
      std::function<SDValue(SDValue)> WidenedOpCB) const override;

  SDValue LowerVPToVVP(SDValue Op, SelectionDAG &DAG, VVPExpansionMode Mode) const;

  SDValue LowerEXTRACT_SUBVECTOR(SDValue Op, SelectionDAG &DAG,
                                 VVPExpansionMode Mode) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVAARG(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalTLSAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerToTLSGeneralDynamicModel(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerToTLSLocalExecModel(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerConstantPool(SDValue Op, SelectionDAG &DAG) const;
  SDValue lowerDYNAMIC_STACKALLOC(SDValue Op, SelectionDAG &DAG) const;
  /// } Custom Lower

  // SjLj
  SDValue LowerEH_SJLJ_SETJMP(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerEH_SJLJ_LONGJMP(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerEH_SJLJ_SETUP_DISPATCH(SDValue Op, SelectionDAG &DAG) const;

  // Custom Operations
  // SDValue CreateConstMask(SDLoc DL, unsigned NumElements, SelectionDAG &DAG,
  // bool IsTrue=true) const; SDValue CreateBroadcast(SDLoc dl, EVT ResTy,
  // SDValue ScaValue, SelectionDAG &DAG, Optional<SDValue> OpVectorLength=None)
  // const; SDValue CreateSeq(SDLoc dl, EVT ResTy, SelectionDAG &DAG,
  // Optional<SDValue> OpVectorLength=None) const;

  // Vector Operations
  // main shuffle handler
  SDValue LowerVectorShuffleOp(SDValue Op, SelectionDAG &DAG,
                               VVPExpansionMode Mode) const;
  SDValue LowerBitcast(SDValue Op, SelectionDAG &DAG) const;
  // SDValue LowerVECTOR_SHUFFLE(SDValue Op, SelectionDAG &DAG, VVPExpansionMode
  // Mode) const; SDValue LowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG,
  // VVPExpansionMode Mode) const;

  SDValue LowerSCALAR_TO_VECTOR(SDValue Op, SelectionDAG &DAG,
                                VVPExpansionMode Mode,
                                VecLenOpt VecLenHint = None) const;
  SDValue LowerMGATHER_MSCATTER(SDValue Op, SelectionDAG &DAG,
                                VVPExpansionMode Mode,
                                VecLenOpt VecLenHint = None) const;

  SDValue LowerMLOAD(SDValue Op, SelectionDAG &DAG, VVPExpansionMode Mode,
                     VecLenOpt VecLenHint = None) const;
  SDValue LowerMSTORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerEXTRACT_VECTOR_ELT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINSERT_VECTOR_ELT(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSTORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVECREDUCE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(llvm::SDValue, llvm::SelectionDAG &) const;
  // SDValue LowerSELECT_CC(llvm::SDValue, llvm::SelectionDAG &) const;//
  // Expanded SDValue LowerVSELECT(llvm::SDValue, llvm::SelectionDAG &) const;

  SDValue ExpandSELECT(SDValue Op, SmallVectorImpl<SDValue> &LegalOperands,
                       EVT LegalResVT, CustomDAG &DAG, SDValue AVL) const;
  SDValue LowerTRUNCATE(llvm::SDValue, llvm::SelectionDAG &) const;

  SDValue TryNarrowExtractVectorLoad(SDNode *ExtractN, SelectionDAG &DAG) const;

  SDValue ExpandToVVP(SDValue Op, SelectionDAG &DAG,
                      VVPExpansionMode Mode) const;
  // main entry point for regular OC to VVP_* ISD expansion
  // Called in TL::ReplaceNodeResults
  // This replaces the standard ISD node with VVP VEISD node(s) with a widened
  // result type.

  SDValue ExpandToSplitVVP(SDValue Op, SelectionDAG &DAG,
                           VVPExpansionMode Mode) const;
  SDValue ExpandToSplitLoadStore(SDValue Op, SelectionDAG &DAG,
                                 VVPExpansionMode Mode) const;
  SDValue ExpandToSplitReduction(SDValue Op, SelectionDAG &DAG,
                                 VVPExpansionMode Mode) const;
  // Split \p Op into two VVP_ ops with the native vector width by splitting
  // the i32 and f32 sub elements of the vector operation.

  SDValue WidenVVPOperation(SDValue Op, SelectionDAG &DAG,
                            VVPExpansionMode Mode) const;
  // Called during TL::LowerOperation
  // This replaces this standard ISD node (or VVP VEISD node) with
  // a VVP VEISD node with a native-width type.

  // expand SETCC opernads directly used in vector arithmeticops
  SDValue LowerSETCCInVectorArithmetic(SDValue Op, SelectionDAG &DAG) const;
  // Should we expand the build vector with shuffles?
  bool
  shouldExpandBuildVectorWithShuffles(EVT VT,
                                      unsigned DefinedValues) const override;

  // Other
  SDValue LowerINTRINSIC_VOID(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINTRINSIC_W_CHAIN(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINTRINSIC_WO_CHAIN(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerATOMIC_FENCE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerATOMIC_LOAD(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerATOMIC_STORE(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVP_VSHIFT(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerCONCAT_VECTOR(SDValue Op, SelectionDAG &DAG) const;
  /// } Custom Lower

  bool isFPImmLegal(const APFloat &Imm, EVT VT,
                    bool ForCodeSize) const override;

  bool ShouldShrinkFPConstant(EVT VT) const override {
    // Do not shrink FP constpool if VT == MVT::f128.
    // (ldd, call _Q_fdtoq) is more expensive than two ldds.
    return VT != MVT::f128;
  }

  /// Returns true if the target allows unaligned memory accesses of the
  /// specified type.
  bool allowsMisalignedMemoryAccesses(EVT VT, unsigned AS, unsigned Align,
                                      MachineMemOperand::Flags Flags,
                                      bool *Fast) const override;
  bool mergeStoresAfterLegalization(EVT) const override { return true; }

  bool canMergeStoresTo(unsigned AddressSpace, EVT MemVT,
                        const SelectionDAG &DAG) const override;

  unsigned getJumpTableEncoding() const override;

  const MCExpr *LowerCustomJumpTableEntry(const MachineJumpTableInfo *MJTI,
                                          const MachineBasicBlock *MBB,
                                          unsigned uid,
                                          MCContext &Ctx) const override;

  bool shouldInsertFencesForAtomic(const Instruction *I) const override {
    // VE uses Release consistency, so need fence for each atomics.
    return true;
  }
  Instruction *emitLeadingFence(IRBuilder<> &Builder, Instruction *Inst,
                                AtomicOrdering Ord) const override;
  Instruction *emitTrailingFence(IRBuilder<> &Builder, Instruction *Inst,
                                 AtomicOrdering Ord) const override;

  AtomicExpansionKind
  shouldExpandAtomicRMWInIR(AtomicRMWInst *AI) const override;

  MachineBasicBlock *expandSelectCC(MachineInstr &MI, MachineBasicBlock *BB,
                                    unsigned BROpcode) const;
  MachineBasicBlock *emitEHSjLjSetJmp(MachineInstr &MI,
                                      MachineBasicBlock *MBB) const;
  MachineBasicBlock *emitEHSjLjLongJmp(MachineInstr &MI,
                                       MachineBasicBlock *MBB) const;
  MachineBasicBlock *EmitSjLjDispatchBlock(MachineInstr &MI,
                                           MachineBasicBlock *BB) const;
  void SetupEntryBlockForSjLj(MachineInstr &MI, MachineBasicBlock *MBB,
                              MachineBasicBlock *DispatchBB, int FI) const;
  void finalizeLowering(MachineFunction &MF) const override;

  // VE supports only vector FMA
  bool isFMAFasterThanFMulAndFAdd(const MachineFunction &MF,
                                  EVT VT) const override {
    return VT.isVector();
  }

#if 0
  // TODO map *ALL* vector types, including EVTs to vregs
  /// Certain combinations of ABIs, Targets and features require that types
  /// are legal for some operations and not for other operations.
  /// For MIPS all vector types must be passed through the integer register set.
  MVT getRegisterTypeForCallingConv(LLVMContext &Context,
                                            CallingConv::ID CC, EVT VT) const override {
  }

  /// Certain targets require unusual breakdowns of certain types. For MIPS,
  /// this occurs when a vector type is used, as vector are passed through the
  /// integer register set.
  unsigned getNumRegistersForCallingConv(LLVMContext &Context,
                                                 CallingConv::ID CC,
                                                 EVT VT) const override {
  }
#endif

  /// Return the preferred vector type legalization action.
  LegalizeTypeAction getPreferredVectorAction(MVT VT) const override;
  bool isVectorMaskType(EVT VT) const;

  /// Target Optimization {

  // SX-Aurora VE s/udiv is 5-9 times slower than multiply.
  bool isIntDivCheap(EVT, AttributeList) const override { return false; }
  bool hasStandaloneRem(EVT) const override { return false; }

  bool isCheapToSpeculateCtlz() const override { return true; }
  bool isCtlzFast() const override { return true; }

  bool convertSetCCLogicToBitwiseLogic(EVT VT) const override {
    return true;
  }

  bool hasAndNot(SDValue Y) const override;

  /// } Target Optimization
};
} // namespace llvm

#endif // VE_ISELLOWERING_H
