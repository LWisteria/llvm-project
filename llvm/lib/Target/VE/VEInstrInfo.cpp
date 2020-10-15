//===-- VEInstrInfo.cpp - VE Instruction Information ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the VE implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "VEInstrInfo.h"
#include "VE.h"
#include "VEMachineFunctionInfo.h"
#include "VESubtarget.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define DEBUG_TYPE "ve-instr-info"

using namespace llvm;

static cl::opt<bool> ShowSpillMessageVec(
  "show-spill-message-vec",
  cl::init(false),
  cl::desc("Enable diagnostic message for spill/restore of vector or vector mask registers."),
  cl::Hidden);

#define GET_INSTRINFO_CTOR_DTOR
#include "VEGenInstrInfo.inc"

// Pin the vtable to this file.
void VEInstrInfo::anchor() {}

VEInstrInfo::VEInstrInfo(VESubtarget &ST)
    : VEGenInstrInfo(VE::ADJCALLSTACKDOWN, VE::ADJCALLSTACKUP), RI(),
      Subtarget(ST) {}

static bool IsIntegerCC(unsigned CC) { return (CC < VECC::CC_AF); }

static VECC::CondCode GetOppositeBranchCondition(VECC::CondCode CC) {
  switch (CC) {
  case VECC::CC_IG:
    return VECC::CC_ILE;
  case VECC::CC_IL:
    return VECC::CC_IGE;
  case VECC::CC_INE:
    return VECC::CC_IEQ;
  case VECC::CC_IEQ:
    return VECC::CC_INE;
  case VECC::CC_IGE:
    return VECC::CC_IL;
  case VECC::CC_ILE:
    return VECC::CC_IG;
  case VECC::CC_AF:
    return VECC::CC_AT;
  case VECC::CC_G:
    return VECC::CC_LENAN;
  case VECC::CC_L:
    return VECC::CC_GENAN;
  case VECC::CC_NE:
    return VECC::CC_EQNAN;
  case VECC::CC_EQ:
    return VECC::CC_NENAN;
  case VECC::CC_GE:
    return VECC::CC_LNAN;
  case VECC::CC_LE:
    return VECC::CC_GNAN;
  case VECC::CC_NUM:
    return VECC::CC_NAN;
  case VECC::CC_NAN:
    return VECC::CC_NUM;
  case VECC::CC_GNAN:
    return VECC::CC_LE;
  case VECC::CC_LNAN:
    return VECC::CC_GE;
  case VECC::CC_NENAN:
    return VECC::CC_EQ;
  case VECC::CC_EQNAN:
    return VECC::CC_NE;
  case VECC::CC_GENAN:
    return VECC::CC_L;
  case VECC::CC_LENAN:
    return VECC::CC_G;
  case VECC::CC_AT:
    return VECC::CC_AF;
  case VECC::UNKNOWN:
    return VECC::UNKNOWN;
  }
  llvm_unreachable("Invalid cond code");
}

// Treat branch relative always like br.l.t as unconditional branch
// instructions.
static bool isUncondBranchOpcode(int Opc) {
  using namespace llvm::VE;

#define BRKIND(NAME) \
    (Opc == NAME ## a || Opc == NAME ## a_nt || Opc == NAME ## a_t)
  return BRKIND(BRCFL) || BRKIND(BRCFW) || BRKIND(BRCFD) || BRKIND(BRCFS);
#undef BRKIND
}

// Treat branch relative conditional like brgt.l.t as conditional branch
// instructions.
static bool isCondBranchOpcode(int Opc) {
  using namespace llvm::VE;

#define BRKIND(NAME) \
    (Opc == NAME ## rr || Opc == NAME ## rr_nt || Opc == NAME ## rr_t || \
     Opc == NAME ## ir || Opc == NAME ## ir_nt || Opc == NAME ## ir_t)
  return BRKIND(BRCFL) || BRKIND(BRCFW) || BRKIND(BRCFD) || BRKIND(BRCFS);
#undef BRKIND
}

// Treat branch always like b.l.t as indirect branch instructions.
static bool isIndirectBranchOpcode(int Opc) {
  using namespace llvm::VE;

#define BRKIND(NAME) \
    (Opc == NAME ## ari || Opc == NAME ## ari_nt || Opc == NAME ## ari_t)
  return BRKIND(BCFL) || BRKIND(BCFW) || BRKIND(BCFD) || BRKIND(BCFS);
#undef BRKIND
}

static void parseCondBranch(MachineInstr *LastInst, MachineBasicBlock *&Target,
                            SmallVectorImpl<MachineOperand> &Cond) {
  Cond.push_back(MachineOperand::CreateImm(LastInst->getOperand(0).getImm()));
  Cond.push_back(LastInst->getOperand(1));
  Cond.push_back(LastInst->getOperand(2));
  Target = LastInst->getOperand(3).getMBB();
}

bool VEInstrInfo::analyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                                MachineBasicBlock *&FBB,
                                SmallVectorImpl<MachineOperand> &Cond,
                                bool AllowModify) const {
  MachineBasicBlock::iterator I = MBB.getLastNonDebugInstr();
  if (I == MBB.end())
    return false;

  if (!isUnpredicatedTerminator(*I))
    return false;

  // Get the last instruction in the block.
  MachineInstr *LastInst = &*I;
  unsigned LastOpc = LastInst->getOpcode();

  // If there is only one terminator instruction, process it.
  if (I == MBB.begin() || !isUnpredicatedTerminator(*--I)) {
    if (isUncondBranchOpcode(LastOpc)) {
      TBB = LastInst->getOperand(0).getMBB();
      return false;
    }
    if (isCondBranchOpcode(LastOpc)) {
      // Block ends with fall-through condbranch.
      parseCondBranch(LastInst, TBB, Cond);
      return false;
    }
    return true; // Can't handle indirect branch.
  }

  // Get the instruction before it if it is a terminator.
  MachineInstr *SecondLastInst = &*I;
  unsigned SecondLastOpc = SecondLastInst->getOpcode();

  // If AllowModify is true and the block ends with two or more unconditional
  // branches, delete all but the first unconditional branch.
  if (AllowModify && isUncondBranchOpcode(LastOpc)) {
    while (isUncondBranchOpcode(SecondLastOpc)) {
      LastInst->eraseFromParent();
      LastInst = SecondLastInst;
      LastOpc = LastInst->getOpcode();
      if (I == MBB.begin() || !isUnpredicatedTerminator(*--I)) {
        // Return now the only terminator is an unconditional branch.
        TBB = LastInst->getOperand(0).getMBB();
        return false;
      }
      SecondLastInst = &*I;
      SecondLastOpc = SecondLastInst->getOpcode();
    }
  }

  // If there are three terminators, we don't know what sort of block this is.
  if (SecondLastInst && I != MBB.begin() && isUnpredicatedTerminator(*--I))
    return true;

  // If the block ends with a B and a Bcc, handle it.
  if (isCondBranchOpcode(SecondLastOpc) && isUncondBranchOpcode(LastOpc)) {
    parseCondBranch(SecondLastInst, TBB, Cond);
    FBB = LastInst->getOperand(0).getMBB();
    return false;
  }

  // If the block ends with two unconditional branches, handle it.  The second
  // one is not executed.
  if (isUncondBranchOpcode(SecondLastOpc) && isUncondBranchOpcode(LastOpc)) {
    TBB = SecondLastInst->getOperand(0).getMBB();
    return false;
  }

  // ...likewise if it ends with an indirect branch followed by an unconditional
  // branch.
  if (isIndirectBranchOpcode(SecondLastOpc) && isUncondBranchOpcode(LastOpc)) {
    I = LastInst;
    if (AllowModify)
      I->eraseFromParent();
    return true;
  }

  // Otherwise, can't handle this.
  return true;
}

unsigned VEInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                   MachineBasicBlock *TBB,
                                   MachineBasicBlock *FBB,
                                   ArrayRef<MachineOperand> Cond,
                                   const DebugLoc &DL, int *BytesAdded) const {
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert((Cond.size() == 3 || Cond.size() == 0) &&
         "VE branch conditions should have three component!");
  assert(!BytesAdded && "code size not handled");
  if (Cond.empty()) {
    // Uncondition branch
    assert(!FBB && "Unconditional branch with multiple successors!");
    BuildMI(&MBB, DL, get(VE::BRCFLa_t))
        .addMBB(TBB);
    return 1;
  }

  // Conditional branch
  //   (BRCFir CC sy sz addr)
  assert(Cond[0].isImm() && Cond[2].isReg() && "not implemented");

  unsigned opc[2];
  const TargetRegisterInfo *TRI = &getRegisterInfo();
  MachineFunction *MF = MBB.getParent();
  const MachineRegisterInfo &MRI = MF->getRegInfo();
  unsigned Reg = Cond[2].getReg();
  if (IsIntegerCC(Cond[0].getImm())) {
    if (TRI->getRegSizeInBits(Reg, MRI) == 32) {
      opc[0] = VE::BRCFWir;
      opc[1] = VE::BRCFWrr;
    } else {
      opc[0] = VE::BRCFLir;
      opc[1] = VE::BRCFLrr;
    }
  } else {
    if (TRI->getRegSizeInBits(Reg, MRI) == 32) {
      opc[0] = VE::BRCFSir;
      opc[1] = VE::BRCFSrr;
    } else {
      opc[0] = VE::BRCFDir;
      opc[1] = VE::BRCFDrr;
    }
  }
  if (Cond[1].isImm()) {
      BuildMI(&MBB, DL, get(opc[0]))
          .add(Cond[0]) // condition code
          .add(Cond[1]) // lhs
          .add(Cond[2]) // rhs
          .addMBB(TBB);
  } else {
      BuildMI(&MBB, DL, get(opc[1]))
          .add(Cond[0])
          .add(Cond[1])
          .add(Cond[2])
          .addMBB(TBB);
  }

  if (!FBB)
    return 1;

  BuildMI(&MBB, DL, get(VE::BRCFLa_t))
      .addMBB(FBB);
  return 2;
}

unsigned VEInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                   int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");

  MachineBasicBlock::iterator I = MBB.end();
  unsigned Count = 0;
  while (I != MBB.begin()) {
    --I;

    if (I->isDebugValue())
      continue;

    if (!isUncondBranchOpcode(I->getOpcode()) &&
        !isCondBranchOpcode(I->getOpcode()))
      break; // Not a branch

    I->eraseFromParent();
    I = MBB.end();
    ++Count;
  }
  return Count;
}

bool VEInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  VECC::CondCode CC = static_cast<VECC::CondCode>(Cond[0].getImm());
  Cond[0].setImm(GetOppositeBranchCondition(CC));
  return false;
}

static bool IsAliasOfSX(Register Reg) {
  return VE::I32RegClass.contains(Reg) || VE::I64RegClass.contains(Reg) ||
         VE::F32RegClass.contains(Reg);
}

static void copyPhysSubRegs(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator I, const DebugLoc &DL,
                            MCRegister DestReg, MCRegister SrcReg, bool KillSrc,
                            const MCInstrDesc &MCID, unsigned int NumSubRegs,
                            const unsigned *SubRegIdx,
                            const TargetRegisterInfo *TRI) {
  MachineInstr *MovMI = nullptr;

  for (unsigned Idx = 0; Idx != NumSubRegs; ++Idx) {
    Register SubDest = TRI->getSubReg(DestReg, SubRegIdx[Idx]);
    Register SubSrc = TRI->getSubReg(SrcReg, SubRegIdx[Idx]);
    assert(SubDest && SubSrc && "Bad sub-register");

    if (MCID.getOpcode() == VE::ORri) {
      // generate "ORri, dest, src, 0" instruction.
      MachineInstrBuilder MIB =
          BuildMI(MBB, I, DL, MCID, SubDest).addReg(SubSrc).addImm(0);
      MovMI = MIB.getInstr();
    } else if (MCID.getOpcode() == VE::ANDMmm) {
      // generate "ANDM, dest, vm0, src" instruction.
      MachineInstrBuilder MIB = BuildMI(MBB, I, DL, MCID, SubDest)
          .addReg(VE::VM0).addReg(SubSrc);
      MovMI = MIB.getInstr();
    } else {
      llvm_unreachable("Unexpected reg-to-reg copy instruction");
    }
  }
  // Add implicit super-register defs and kills to the last MovMI.
  MovMI->addRegisterDefined(DestReg, TRI);
  if (KillSrc)
    MovMI->addRegisterKilled(SrcReg, TRI, true);
}

void VEInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I, const DebugLoc &DL,
                              MCRegister DestReg, MCRegister SrcReg,
                              bool KillSrc) const {

  if (IsAliasOfSX(SrcReg) && IsAliasOfSX(DestReg)) {
    BuildMI(MBB, I, DL, get(VE::ORri), DestReg)
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addImm(0);
  } else if (VE::V64RegClass.contains(DestReg, SrcReg)) {
    // Generate following instructions
    //   %sw16 = LEA32zii 256
    //   VORmvl %dest, (0)1, %src, %sw16
    // TODO: reuse a register if vl is already assigned to a register
    // FIXME: it would be better to scavenge a register here instead of
    // reserving SX16 all of the time.
    const TargetRegisterInfo *TRI = &getRegisterInfo();
    unsigned TmpReg = VE::SX16;
    unsigned SubTmp = TRI->getSubReg(TmpReg, VE::sub_i32);
    BuildMI(MBB, I, DL, get(VE::LEAzii), TmpReg)
        .addImm(0).addImm(0).addImm(256);
    MachineInstrBuilder MIB = BuildMI(MBB, I, DL, get(VE::VORmvl), DestReg)
        .addImm(M1(0)) // Represent (0)1.
        .addReg(SrcReg, getKillRegState(KillSrc))
        .addReg(SubTmp, getKillRegState(true));
    MIB.getInstr()->addRegisterKilled(TmpReg, TRI, true);
  } else if (VE::VMRegClass.contains(DestReg, SrcReg))
    BuildMI(MBB, I, DL, get(VE::ANDMmm), DestReg)
        .addReg(VE::VM0)
        .addReg(SrcReg, getKillRegState(KillSrc));
  else if (VE::VM512RegClass.contains(DestReg, SrcReg)) {
    // Use two instructions.
    const unsigned subRegIdx[] = { VE::sub_vm_even, VE::sub_vm_odd };
    unsigned int numSubRegs = 2;
    copyPhysSubRegs(MBB, I, DL, DestReg, SrcReg, KillSrc, get(VE::ANDMmm),
                    numSubRegs, subRegIdx, &getRegisterInfo());
  } else if (VE::F128RegClass.contains(DestReg, SrcReg)) {
    // Use two instructions.
    const unsigned SubRegIdx[] = {VE::sub_even, VE::sub_odd};
    unsigned int NumSubRegs = 2;
    copyPhysSubRegs(MBB, I, DL, DestReg, SrcReg, KillSrc, get(VE::ORri),
                    NumSubRegs, SubRegIdx, &getRegisterInfo());
  } else {
    const TargetRegisterInfo *TRI = &getRegisterInfo();
    dbgs() << "Impossible reg-to-reg copy from " << printReg(SrcReg, TRI)
           << " to " << printReg(DestReg, TRI) << "\n";
    llvm_unreachable("Impossible reg-to-reg copy");
  }
}

/// isLoadFromStackSlot - If the specified machine instruction is a direct
/// load from a stack slot, return the virtual or physical register number of
/// the destination along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than loading from the stack slot.
unsigned VEInstrInfo::isLoadFromStackSlot(const MachineInstr &MI,
                                          int &FrameIndex) const {
  if (MI.getOpcode() == VE::LDrii ||            // I64
      MI.getOpcode() == VE::LDLSXrii ||         // I32
      MI.getOpcode() == VE::LDUrii ||           // F32
      MI.getOpcode() == VE::LDQrii ||           // F128 (pseudo)
      MI.getOpcode() == VE::LDVRrii ||          // V64 (pseudo)
      MI.getOpcode() == VE::LDVMrii ||          // VM (pseudo)
      MI.getOpcode() == VE::LDVM512rii          // VM512 (pseudo)
  ) {
    if (MI.getOperand(1).isFI() && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0 && MI.getOperand(3).isImm() &&
        MI.getOperand(3).getImm() == 0) {
      FrameIndex = MI.getOperand(1).getIndex();
      return MI.getOperand(0).getReg();
    }
  }
  return 0;
}

/// isStoreToStackSlot - If the specified machine instruction is a direct
/// store to a stack slot, return the virtual or physical register number of
/// the source reg along with the FrameIndex of the loaded stack slot.  If
/// not, return 0.  This predicate must return 0 if the instruction has
/// any side effects other than storing to the stack slot.
unsigned VEInstrInfo::isStoreToStackSlot(const MachineInstr &MI,
                                         int &FrameIndex) const {
  if (MI.getOpcode() == VE::STrii ||            // I64
      MI.getOpcode() == VE::STLrii ||           // I32
      MI.getOpcode() == VE::STUrii ||           // F32
      MI.getOpcode() == VE::STQrii ||           // F128 (pseudo)
      MI.getOpcode() == VE::STVRrii ||          // V64 (pseudo)
      MI.getOpcode() == VE::STVMrii ||          // VM (pseudo)
      MI.getOpcode() == VE::STVM512rii          // VM512 (pseudo)
  ) {
    if (MI.getOperand(0).isFI() && MI.getOperand(1).isImm() &&
        MI.getOperand(1).getImm() == 0 && MI.getOperand(2).isImm() &&
        MI.getOperand(2).getImm() == 0) {
      FrameIndex = MI.getOperand(0).getIndex();
      return MI.getOperand(3).getReg();
    }
  }
  return 0;
}

void VEInstrInfo::storeRegToStackSlot(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator I,
                                      Register SrcReg, bool isKill, int FI,
                                      const TargetRegisterClass *RC,
                                      const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  if (ShowSpillMessageVec) {
    if (RC == &VE::V64RegClass) {
      dbgs() << "spill " << printReg(SrcReg, TRI) << " - V64\n";
    } else if (RC == &VE::VMRegClass) {
      dbgs() << "spill " << printReg(SrcReg, TRI) << " - VM\n";
    } else if (VE::VM512RegClass.hasSubClassEq(RC)) {
      dbgs() << "spill " << printReg(SrcReg, TRI) << " - VM512\n";
    }
  }

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOStore,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  // On the order of operands here: think "[FrameIdx + 0] = SrcReg".
  if (RC == &VE::I64RegClass) {
    BuildMI(MBB, I, DL, get(VE::STrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else if (RC == &VE::I32RegClass) {
    BuildMI(MBB, I, DL, get(VE::STLrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else if (RC == &VE::F32RegClass) {
    BuildMI(MBB, I, DL, get(VE::STUrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else if (VE::F128RegClass.hasSubClassEq(RC)) {
    BuildMI(MBB, I, DL, get(VE::STQrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else if (RC == &VE::V64RegClass) {
    BuildMI(MBB, I, DL, get(VE::STVRrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addImm(256)
        .addMemOperand(MMO);
  } else if (RC == &VE::VMRegClass) {
    BuildMI(MBB, I, DL, get(VE::STVMrii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else if (VE::VM512RegClass.hasSubClassEq(RC)) {
    BuildMI(MBB, I, DL, get(VE::STVM512rii))
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addReg(SrcReg, getKillRegState(isKill))
        .addMemOperand(MMO);
  } else
    report_fatal_error("Can't store this register to stack slot");
}

void VEInstrInfo::loadRegFromStackSlot(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator I,
                                       Register DestReg, int FI,
                                       const TargetRegisterClass *RC,
                                       const TargetRegisterInfo *TRI) const {
  DebugLoc DL;
  if (I != MBB.end())
    DL = I->getDebugLoc();

  if (ShowSpillMessageVec) {
    if (RC == &VE::V64RegClass) {
      dbgs() << "restore " << printReg(DestReg, TRI) << " - V64\n";
    } else if (RC == &VE::VMRegClass) {
      dbgs() << "restore " << printReg(DestReg, TRI) << " - VM\n";
    } else if (VE::VM512RegClass.hasSubClassEq(RC)) {
      dbgs() << "restore " << printReg(DestReg, TRI) << " - VM512\n";
    }
  }

  MachineFunction *MF = MBB.getParent();
  const MachineFrameInfo &MFI = MF->getFrameInfo();
  MachineMemOperand *MMO = MF->getMachineMemOperand(
      MachinePointerInfo::getFixedStack(*MF, FI), MachineMemOperand::MOLoad,
      MFI.getObjectSize(FI), MFI.getObjectAlign(FI));

  if (RC == &VE::I64RegClass) {
    BuildMI(MBB, I, DL, get(VE::LDrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (RC == &VE::I32RegClass) {
    BuildMI(MBB, I, DL, get(VE::LDLSXrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (RC == &VE::F32RegClass) {
    BuildMI(MBB, I, DL, get(VE::LDUrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (VE::F128RegClass.hasSubClassEq(RC)) {
    BuildMI(MBB, I, DL, get(VE::LDQrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (RC == &VE::V64RegClass) {
    BuildMI(MBB, I, DL, get(VE::LDVRrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addImm(256)
        .addMemOperand(MMO);
  } else if (RC == &VE::VMRegClass) {
    BuildMI(MBB, I, DL, get(VE::LDVMrii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else if (VE::VM512RegClass.hasSubClassEq(RC)) {
    BuildMI(MBB, I, DL, get(VE::LDVM512rii), DestReg)
        .addFrameIndex(FI)
        .addImm(0)
        .addImm(0)
        .addMemOperand(MMO);
  } else
    report_fatal_error("Can't load this register from stack slot");
}

Register VEInstrInfo::getGlobalBaseReg(MachineFunction *MF) const {
  VEMachineFunctionInfo *VEFI = MF->getInfo<VEMachineFunctionInfo>();
  Register GlobalBaseReg = VEFI->getGlobalBaseReg();
  if (GlobalBaseReg != 0)
    return GlobalBaseReg;

  // We use %s15 (%got) as a global base register
  GlobalBaseReg = VE::SX15;

  // Insert a pseudo instruction to set the GlobalBaseReg into the first
  // MBB of the function
  MachineBasicBlock &FirstMBB = MF->front();
  MachineBasicBlock::iterator MBBI = FirstMBB.begin();
  DebugLoc dl;
  BuildMI(FirstMBB, MBBI, dl, get(VE::GETGOT), GlobalBaseReg);
  VEFI->setGlobalBaseReg(GlobalBaseReg);
  return GlobalBaseReg;
}

static int GetVM512Upper(int no)
{
    return (no - VE::VMP0) * 2 + VE::VM0;
}

static int GetVM512Lower(int no)
{
    return GetVM512Upper(no) + 1;
}

static void buildVMRInst(MachineInstr& MI, const MCInstrDesc& MCID) {
  MachineBasicBlock* MBB = MI.getParent();
  DebugLoc dl = MI.getDebugLoc();

  unsigned VMXu = GetVM512Upper(MI.getOperand(0).getReg());
  unsigned VMXl = GetVM512Lower(MI.getOperand(0).getReg());
  unsigned VMYu = GetVM512Upper(MI.getOperand(1).getReg());
  unsigned VMYl = GetVM512Lower(MI.getOperand(1).getReg());

  switch (MI.getOpcode()) {
  default: {
      unsigned VMZu = GetVM512Upper(MI.getOperand(2).getReg());
      unsigned VMZl = GetVM512Lower(MI.getOperand(2).getReg());
      BuildMI(*MBB, MI, dl, MCID).addDef(VMXu).addUse(VMYu).addUse(VMZu);
      BuildMI(*MBB, MI, dl, MCID).addDef(VMXl).addUse(VMYl).addUse(VMZl);
      break;
  }
  case VE::veoldNEGMy:
      BuildMI(*MBB, MI, dl, MCID).addDef(VMXu).addUse(VMYu);
      BuildMI(*MBB, MI, dl, MCID).addDef(VMXl).addUse(VMYl);
      break;
  }
  MI.eraseFromParent();
}

static void expandPseudoVFMK_VL(const TargetInstrInfo& TI, MachineInstr& MI)
{
    // replace to pvfmk.w.up and pvfmk.w.lo
    // replace to pvfmk.s.up and pvfmk.s.lo

    std::map<int, std::vector<int>> map = {
      {VE::VFMKyal, {VE::VFMKLal, VE::VFMKLal}},
      {VE::VFMKynal, {VE::VFMKLnal, VE::VFMKLnal}},
      {VE::VFMKWyvl, {VE::PVFMKWUPvl, VE::PVFMKWLOvl}},
      {VE::VFMKWyvyl, {VE::PVFMKWUPvml, VE::PVFMKWLOvml}},
      {VE::VFMKSyvl, {VE::PVFMKSUPvl, VE::PVFMKSLOvl}},
      {VE::VFMKSyvyl, {VE::PVFMKSUPvml, VE::PVFMKSLOvml}},

      {VE::veoldVFMKyal, {VE::veoldVFMKLal, VE::veoldVFMKLal}},
      {VE::veoldVFMKynal, {VE::veoldVFMKLnal, VE::veoldVFMKLnal}},
      {VE::veoldVFMKWyvl, {VE::veoldPVFMKWUPxvl, VE::veoldPVFMKWLOxvl}},
      {VE::veoldVFMKWyvyl, {VE::veoldPVFMKWUPxvxl, VE::veoldPVFMKWLOxvxl}},
      {VE::veoldVFMKSyvl, {VE::veoldPVFMKSUPxvl, VE::veoldPVFMKSLOxvl}},
      {VE::veoldVFMKSyvyl, {VE::veoldPVFMKSUPxvxl, VE::veoldPVFMKSLOxvxl}},
    };

    unsigned Opcode = MI.getOpcode();

    if (map.find(Opcode) == map.end()) {
      report_fatal_error("unexpected opcode for pseudo vfmk");
    }

    unsigned OpcodeUpper = map[Opcode][0];
    unsigned OpcodeLower = map[Opcode][1];

    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc dl = MI.getDebugLoc();
    MachineInstrBuilder Bu = BuildMI(*MBB, MI, dl, TI.get(OpcodeUpper));
    MachineInstrBuilder Bl = BuildMI(*MBB, MI, dl, TI.get(OpcodeLower));

    // VM512
    Bu.addReg(GetVM512Upper(MI.getOperand(0).getReg()));
    Bl.addReg(GetVM512Lower(MI.getOperand(0).getReg()));

    if (MI.getNumExplicitOperands() == 2) { // _Ml: VM512, VL
      // VL
      Bu.addReg(MI.getOperand(1).getReg());
      Bl.addReg(MI.getOperand(1).getReg());
    } else if (MI.getNumExplicitOperands() == 4) { // _Mvl: VM512, CC, VR, VL
      // CC
      Bu.addImm(MI.getOperand(1).getImm());
      Bl.addImm(MI.getOperand(1).getImm());
      // VR
      Bu.addReg(MI.getOperand(2).getReg());
      Bl.addReg(MI.getOperand(2).getReg());
      // VL
      Bu.addReg(MI.getOperand(3).getReg());
      Bl.addReg(MI.getOperand(3).getReg());
    } else if (MI.getNumExplicitOperands() == 5) { // _MvMl: VM512, CC, VR, VM512, VL
      // CC
      Bu.addImm(MI.getOperand(1).getImm());
      Bl.addImm(MI.getOperand(1).getImm());
      // VR
      Bu.addReg(MI.getOperand(2).getReg());
      Bl.addReg(MI.getOperand(2).getReg());
      // VM512
      Bu.addReg(GetVM512Upper(MI.getOperand(3).getReg()));
      Bl.addReg(GetVM512Lower(MI.getOperand(3).getReg()));
      // VL
      Bu.addReg(MI.getOperand(4).getReg());
      Bl.addReg(MI.getOperand(4).getReg());
    } else {
      report_fatal_error("unexpected number of operands for pvfmk");
    }

    MI.eraseFromParent();
}
#if 0
static void expandPseudoVFMK_VL(const TargetInstrInfo& TI, MachineInstr& MI)
{
    // replace to pvfmk.w.up and pvfmk.w.lo
    // replace to pvfmk.s.up and pvfmk.s.lo

    std::map<int, std::vector<int>> map = {
      {VE::veoldPVFMKATl, {VE::veoldPVFMKWUPATl, VE::veoldPVFMKWLOATl}},
      {VE::veoldPVFMKAFl, {VE::veoldPVFMKWUPAFl, VE::veoldPVFMKWLOAFl}},
      {VE::veoldPVFMKWGTvl, {VE::veoldPVFMKWUPGTvl, VE::veoldPVFMKWLOGTvl}},
      {VE::veoldPVFMKWLTvl, {VE::veoldPVFMKWUPLTvl, VE::veoldPVFMKWLOLTvl}},
      {VE::veoldPVFMKWNEvl, {VE::veoldPVFMKWUPNEvl, VE::veoldPVFMKWLONEvl}},
      {VE::veoldPVFMKWEQvl, {VE::veoldPVFMKWUPEQvl, VE::veoldPVFMKWLOEQvl}},
      {VE::veoldPVFMKWGEvl, {VE::veoldPVFMKWUPGEvl, VE::veoldPVFMKWLOGEvl}},
      {VE::veoldPVFMKWLEvl, {VE::veoldPVFMKWUPLEvl, VE::veoldPVFMKWLOLEvl}},
      {VE::veoldPVFMKWNUMvl, {VE::veoldPVFMKWUPNUMvl, VE::veoldPVFMKWLONUMvl}},
      {VE::veoldPVFMKWNANvl, {VE::veoldPVFMKWUPNANvl, VE::veoldPVFMKWLONANvl}},
      {VE::veoldPVFMKWGTNANvl, {VE::veoldPVFMKWUPGTNANvl, VE::veoldPVFMKWLOGTNANvl}},
      {VE::veoldPVFMKWLTNANvl, {VE::veoldPVFMKWUPLTNANvl, VE::veoldPVFMKWLOLTNANvl}},
      {VE::veoldPVFMKWNENANvl, {VE::veoldPVFMKWUPNENANvl, VE::veoldPVFMKWLONENANvl}},
      {VE::veoldPVFMKWEQNANvl, {VE::veoldPVFMKWUPEQNANvl, VE::veoldPVFMKWLOEQNANvl}},
      {VE::veoldPVFMKWGENANvl, {VE::veoldPVFMKWUPGENANvl, VE::veoldPVFMKWLOGENANvl}},
      {VE::veoldPVFMKWLENANvl, {VE::veoldPVFMKWUPLENANvl, VE::veoldPVFMKWLOLENANvl}},

      {VE::veoldPVFMKWGTvxl, {VE::veoldPVFMKWUPGTvxl, VE::veoldPVFMKWLOGTvxl}},
      {VE::veoldPVFMKWLTvxl, {VE::veoldPVFMKWUPLTvxl, VE::veoldPVFMKWLOLTvxl}},
      {VE::veoldPVFMKWNEvxl, {VE::veoldPVFMKWUPNEvxl, VE::veoldPVFMKWLONEvxl}},
      {VE::veoldPVFMKWEQvxl, {VE::veoldPVFMKWUPEQvxl, VE::veoldPVFMKWLOEQvxl}},
      {VE::veoldPVFMKWGEvxl, {VE::veoldPVFMKWUPGEvxl, VE::veoldPVFMKWLOGEvxl}},
      {VE::veoldPVFMKWLEvxl, {VE::veoldPVFMKWUPLEvxl, VE::veoldPVFMKWLOLEvxl}},
      {VE::veoldPVFMKWNUMvxl, {VE::veoldPVFMKWUPNUMvxl, VE::veoldPVFMKWLONUMvxl}},
      {VE::veoldPVFMKWNANvxl, {VE::veoldPVFMKWUPNANvxl, VE::veoldPVFMKWLONANvxl}},
      {VE::veoldPVFMKWGTNANvxl, {VE::veoldPVFMKWUPGTNANvxl, VE::veoldPVFMKWLOGTNANvxl}},
      {VE::veoldPVFMKWLTNANvxl, {VE::veoldPVFMKWUPLTNANvxl, VE::veoldPVFMKWLOLTNANvxl}},
      {VE::veoldPVFMKWNENANvxl, {VE::veoldPVFMKWUPNENANvxl, VE::veoldPVFMKWLONENANvxl}},
      {VE::veoldPVFMKWEQNANvxl, {VE::veoldPVFMKWUPEQNANvxl, VE::veoldPVFMKWLOEQNANvxl}},
      {VE::veoldPVFMKWGENANvxl, {VE::veoldPVFMKWUPGENANvxl, VE::veoldPVFMKWLOGENANvxl}},
      {VE::veoldPVFMKWLENANvxl, {VE::veoldPVFMKWUPLENANvxl, VE::veoldPVFMKWLOLENANvxl}},

      {VE::veoldPVFMKSGTvl, {VE::veoldPVFMKSUPGTvl, VE::veoldPVFMKSLOGTvl}},
      {VE::veoldPVFMKSGTvxl, {VE::veoldPVFMKSUPGTvxl, VE::veoldPVFMKSLOGTvxl}},
      {VE::veoldPVFMKSLTvl, {VE::veoldPVFMKSUPLTvl, VE::veoldPVFMKSLOLTvl}},
      {VE::veoldPVFMKSNEvl, {VE::veoldPVFMKSUPNEvl, VE::veoldPVFMKSLONEvl}},
      {VE::veoldPVFMKSEQvl, {VE::veoldPVFMKSUPEQvl, VE::veoldPVFMKSLOEQvl}},
      {VE::veoldPVFMKSGEvl, {VE::veoldPVFMKSUPGEvl, VE::veoldPVFMKSLOGEvl}},
      {VE::veoldPVFMKSLEvl, {VE::veoldPVFMKSUPLEvl, VE::veoldPVFMKSLOLEvl}},
      {VE::veoldPVFMKSNUMvl, {VE::veoldPVFMKSUPNUMvl, VE::veoldPVFMKSLONUMvl}},
      {VE::veoldPVFMKSNANvl, {VE::veoldPVFMKSUPNANvl, VE::veoldPVFMKSLONANvl}},
      {VE::veoldPVFMKSGTNANvl, {VE::veoldPVFMKSUPGTNANvl, VE::veoldPVFMKSLOGTNANvl}},
      {VE::veoldPVFMKSLTNANvl, {VE::veoldPVFMKSUPLTNANvl, VE::veoldPVFMKSLOLTNANvl}},
      {VE::veoldPVFMKSNENANvl, {VE::veoldPVFMKSUPNENANvl, VE::veoldPVFMKSLONENANvl}},
      {VE::veoldPVFMKSEQNANvl, {VE::veoldPVFMKSUPEQNANvl, VE::veoldPVFMKSLOEQNANvl}},
      {VE::veoldPVFMKSGENANvl, {VE::veoldPVFMKSUPGENANvl, VE::veoldPVFMKSLOGENANvl}},
      {VE::veoldPVFMKSLENANvl, {VE::veoldPVFMKSUPLENANvl, VE::veoldPVFMKSLOLENANvl}},

      {VE::veoldPVFMKSGTvxl, {VE::veoldPVFMKSUPGTvxl, VE::veoldPVFMKSLOGTvxl}},
      {VE::veoldPVFMKSLTvxl, {VE::veoldPVFMKSUPLTvxl, VE::veoldPVFMKSLOLTvxl}},
      {VE::veoldPVFMKSNEvxl, {VE::veoldPVFMKSUPNEvxl, VE::veoldPVFMKSLONEvxl}},
      {VE::veoldPVFMKSEQvxl, {VE::veoldPVFMKSUPEQvxl, VE::veoldPVFMKSLOEQvxl}},
      {VE::veoldPVFMKSGEvxl, {VE::veoldPVFMKSUPGEvxl, VE::veoldPVFMKSLOGEvxl}},
      {VE::veoldPVFMKSLEvxl, {VE::veoldPVFMKSUPLEvxl, VE::veoldPVFMKSLOLEvxl}},
      {VE::veoldPVFMKSNUMvxl, {VE::veoldPVFMKSUPNUMvxl, VE::veoldPVFMKSLONUMvxl}},
      {VE::veoldPVFMKSNANvxl, {VE::veoldPVFMKSUPNANvxl, VE::veoldPVFMKSLONANvxl}},
      {VE::veoldPVFMKSGTNANvxl, {VE::veoldPVFMKSUPGTNANvxl, VE::veoldPVFMKSLOGTNANvxl}},
      {VE::veoldPVFMKSLTNANvxl, {VE::veoldPVFMKSUPLTNANvxl, VE::veoldPVFMKSLOLTNANvxl}},
      {VE::veoldPVFMKSNENANvxl, {VE::veoldPVFMKSUPNENANvxl, VE::veoldPVFMKSLONENANvxl}},
      {VE::veoldPVFMKSEQNANvxl, {VE::veoldPVFMKSUPEQNANvxl, VE::veoldPVFMKSLOEQNANvxl}},
      {VE::veoldPVFMKSGENANvxl, {VE::veoldPVFMKSUPGENANvxl, VE::veoldPVFMKSLOGENANvxl}},
      {VE::veoldPVFMKSLENANvxl, {VE::veoldPVFMKSUPLENANvxl, VE::veoldPVFMKSLOLENANvxl}},
    };

    unsigned Opcode = MI.getOpcode();

    if (map.find(Opcode) == map.end()) {
      report_fatal_error("unexpected opcode for pvfmk");
    }

    unsigned OpcodeUpper = map[Opcode][0];
    unsigned OpcodeLower = map[Opcode][1];

    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc dl = MI.getDebugLoc();
    MachineInstrBuilder Bu = BuildMI(*MBB, MI, dl, TI.get(OpcodeUpper));
    MachineInstrBuilder Bl = BuildMI(*MBB, MI, dl, TI.get(OpcodeLower));

    // VM512
    Bu.addReg(GetVM512Upper(MI.getOperand(0).getReg()));
    Bl.addReg(GetVM512Lower(MI.getOperand(0).getReg()));

    if (MI.getNumExplicitOperands() == 2) { // _Ml: VM512, VL
      // VL
      Bu.addReg(MI.getOperand(1).getReg());
      Bl.addReg(MI.getOperand(1).getReg());
    } else if (MI.getNumExplicitOperands() == 3) { // _Mvl: VM512, VR, VL
      // VR
      Bu.addReg(MI.getOperand(1).getReg());
      Bl.addReg(MI.getOperand(1).getReg());
      // VL
      Bu.addReg(MI.getOperand(2).getReg());
      Bl.addReg(MI.getOperand(2).getReg());
    } else if (MI.getNumExplicitOperands() == 4) { // _MvMl: VM512, VR, VM512, VL
      // VR
      Bu.addReg(MI.getOperand(1).getReg());
      Bl.addReg(MI.getOperand(1).getReg());
      // VM512
      Bu.addReg(GetVM512Upper(MI.getOperand(2).getReg()));
      Bl.addReg(GetVM512Lower(MI.getOperand(2).getReg()));
      // VL
      Bu.addReg(MI.getOperand(3).getReg());
      Bl.addReg(MI.getOperand(3).getReg());
    } else {
      report_fatal_error("unexpected number of operands for pvfmk");
    }

    MI.eraseFromParent();
}
#endif

bool VEInstrInfo::expandPostRAPseudo(MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  case VE::EXTEND_STACK: {
    return expandExtendStackPseudo(MI);
  }
  case VE::EXTEND_STACK_GUARD: {
    MI.eraseFromParent(); // The pseudo instruction is gone now.
    return true;
  }
  case TargetOpcode::LOAD_STACK_GUARD: {
    assert(Subtarget.isTargetLinux() &&
           "Only Linux target is expected to contain LOAD_STACK_GUARD");
    report_fatal_error("expandPostRAPseudo for LOAD_STACK_GUARD is not implemented yet");
#if 0
    // offsetof(tcbhead_t, stack_guard) from sysdeps/sparc/nptl/tls.h in glibc.
    const int64_t Offset = Subtarget.is64Bit() ? 0x28 : 0x14;
    MI.setDesc(get(Subtarget.is64Bit() ? SP::LDXri : SP::LDri));
    MachineInstrBuilder(*MI.getParent()->getParent(), MI)
        .addReg(SP::G7)
        .addImm(Offset);
    return true;
#endif
  }
  case VE::GETSTACKTOP: {
    return expandGetStackTopPseudo(MI);
  }
#if 0
  case VE::VE_SELECT: {
    // (VESelect $dst, $CC, $condVal, $trueVal, $dst)
    //   -> (CMOVrr $dst, condCode, $trueVal, $condVal)
    // cmov.$df.$cf $dst, $trueval, $cond

    assert(MI.getOperand(0).getReg() == MI.getOperand(4).getReg());

    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc dl = MI.getDebugLoc();
    BuildMI(*MBB, MI, dl, get(VE::CMOVWrr))
      .addReg(MI.getOperand(0).getReg())
      .addImm(MI.getOperand(1).getImm())
      .addReg(MI.getOperand(3).getReg())
      .addReg(MI.getOperand(2).getReg());

    MI.eraseFromParent();
    return true;
  }
#endif

  case VE::veoldANDMyy: buildVMRInst(MI, get(VE::ANDMmm)); return true;
  case VE::veoldORMyy:  buildVMRInst(MI, get(VE::ORMmm)); return true;
  case VE::veoldXORMyy: buildVMRInst(MI, get(VE::XORMmm)); return true;
  case VE::veoldEQVMyy: buildVMRInst(MI, get(VE::EQVMmm)); return true;
  case VE::veoldNNDMyy: buildVMRInst(MI, get(VE::NNDMmm)); return true;
  case VE::veoldNEGMy: buildVMRInst(MI, get(VE::NEGMm)); return true;

  case VE::LVMyir:
  case VE::LVMyim:
  case VE::LVMyir_y:
  case VE::LVMyim_y: {
    unsigned VMXu = GetVM512Upper(MI.getOperand(0).getReg());
    unsigned VMXl = GetVM512Lower(MI.getOperand(0).getReg());
    int64_t Imm = MI.getOperand(1).getImm();
    bool IsSrcReg =
        MI.getOpcode() == VE::LVMyir || MI.getOpcode() == VE::LVMyir_y;
    unsigned Src = IsSrcReg ? MI.getOperand(2).getReg() : VE::NoRegister;
    int64_t MImm = IsSrcReg ? 0 : MI.getOperand(2).getImm();
    bool KillSrc = IsSrcReg ? MI.getOperand(2).isKill() : false;
    unsigned VMX = VMXl;
    if (Imm >= 4) {
        VMX = VMXu;
        Imm -= 4;
    }
    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc DL = MI.getDebugLoc();
    switch (MI.getOpcode()) {
    case VE::LVMyir:
      BuildMI(*MBB, MI, DL, get(VE::LVMir))
        .addDef(VMX)
        .addImm(Imm)
        .addReg(Src, getKillRegState(KillSrc));
      break;
    case VE::LVMyim:
      BuildMI(*MBB, MI, DL, get(VE::LVMim))
        .addDef(VMX)
        .addImm(Imm)
        .addImm(MImm);
      break;
    case VE::LVMyir_y:
      assert(MI.getOperand(0).getReg() == MI.getOperand(3).getReg() &&
             "LVMyir_y has different register in 3rd operand");
      BuildMI(*MBB, MI, DL, get(VE::LVMir_m))
        .addDef(VMX)
        .addImm(Imm)
        .addReg(Src, getKillRegState(KillSrc))
        .addReg(VMX);
      break;
    case VE::LVMyim_y:
      assert(MI.getOperand(0).getReg() == MI.getOperand(3).getReg() &&
             "LVMyim_y has different register in 3rd operand");
      BuildMI(*MBB, MI, DL, get(VE::LVMim_m))
        .addDef(VMX)
        .addImm(Imm)
        .addImm(MImm)
        .addReg(VMX);
      break;
    }
    MI.eraseFromParent();
    return true;
  }
  case VE::veoldLVMyir_y: {
    unsigned VMXu = GetVM512Upper(MI.getOperand(0).getReg());
    unsigned VMXl = GetVM512Lower(MI.getOperand(0).getReg());
    unsigned VMDu = GetVM512Upper(MI.getOperand(3).getReg());
    unsigned VMDl = GetVM512Upper(MI.getOperand(3).getReg());
    int64_t Imm = MI.getOperand(1).getImm();
    unsigned VMX = VMXl;
    unsigned VMD = VMDl;
    if (Imm >= 4) {
        VMX = VMXu;
        VMD = VMDu;
        Imm -= 4;
    }
    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc DL = MI.getDebugLoc();
    BuildMI(*MBB, MI, DL, get(VE::LVMir_m))
      .addDef(VMX)
      .addImm(Imm)
      .addReg(MI.getOperand(2).getReg())
      .addReg(VMD);
    MI.eraseFromParent();
    return true;
  }
  case VE::SVMyi:
  case VE::veoldSVMyi: {
    unsigned Dest = MI.getOperand(0).getReg();
    unsigned VMZu = GetVM512Upper(MI.getOperand(1).getReg());
    unsigned VMZl = GetVM512Lower(MI.getOperand(1).getReg());
    bool KillSrc = MI.getOperand(1).isKill();
    int64_t Imm = MI.getOperand(2).getImm();
    unsigned VMZ = VMZl;
    if (Imm >= 4) {
        VMZ = VMZu;
        Imm -= 4;
    }
    MachineBasicBlock* MBB = MI.getParent();
    DebugLoc DL = MI.getDebugLoc();
    MachineInstrBuilder MIB = BuildMI(*MBB, MI, DL, get(VE::SVMmi), Dest)
      .addReg(VMZ)
      .addImm(Imm);
    MachineInstr *Inst = MIB.getInstr();
    MI.eraseFromParent();
    if (KillSrc) {
      const TargetRegisterInfo *TRI = &getRegisterInfo();
      Inst->addRegisterKilled(MI.getOperand(1).getReg(), TRI, true);
    }
    return true;
  }
  case VE::VFMKyal:
  case VE::VFMKynal:
  case VE::VFMKWyvl: case VE::VFMKWyvyl:
  case VE::VFMKSyvl: case VE::VFMKSyvyl:
  case VE::veoldVFMKyal:
  case VE::veoldVFMKynal:
  case VE::veoldVFMKWyvl: case VE::veoldVFMKWyvyl:
  case VE::veoldVFMKSyvl: case VE::veoldVFMKSyvyl:
    expandPseudoVFMK_VL(*this, MI);
#if 0
  case VE::veoldPVFMKATl:
  case VE::veoldPVFMKAFl:
  case VE::veoldPVFMKWGTvl: case VE::veoldPVFMKWGTvxl:
  case VE::veoldPVFMKWLTvl: case VE::veoldPVFMKWLTvxl:
  case VE::veoldPVFMKWNEvl: case VE::veoldPVFMKWNEvxl:
  case VE::veoldPVFMKWEQvl: case VE::veoldPVFMKWEQvxl:
  case VE::veoldPVFMKWGEvl: case VE::veoldPVFMKWGEvxl:
  case VE::veoldPVFMKWLEvl: case VE::veoldPVFMKWLEvxl:
  case VE::veoldPVFMKWNUMvl: case VE::veoldPVFMKWNUMvxl:
  case VE::veoldPVFMKWNANvl: case VE::veoldPVFMKWNANvxl:
  case VE::veoldPVFMKWGTNANvl: case VE::veoldPVFMKWGTNANvxl:
  case VE::veoldPVFMKWLTNANvl: case VE::veoldPVFMKWLTNANvxl:
  case VE::veoldPVFMKWNENANvl: case VE::veoldPVFMKWNENANvxl:
  case VE::veoldPVFMKWEQNANvl: case VE::veoldPVFMKWEQNANvxl:
  case VE::veoldPVFMKWGENANvl: case VE::veoldPVFMKWGENANvxl:
  case VE::veoldPVFMKWLENANvl: case VE::veoldPVFMKWLENANvxl:
  case VE::veoldPVFMKSGTvl: case VE::veoldPVFMKSGTvxl:
  case VE::veoldPVFMKSLTvl: case VE::veoldPVFMKSLTvxl:
  case VE::veoldPVFMKSNEvl: case VE::veoldPVFMKSNEvxl:
  case VE::veoldPVFMKSEQvl: case VE::veoldPVFMKSEQvxl:
  case VE::veoldPVFMKSGEvl: case VE::veoldPVFMKSGEvxl:
  case VE::veoldPVFMKSLEvl: case VE::veoldPVFMKSLEvxl:
  case VE::veoldPVFMKSNUMvl: case VE::veoldPVFMKSNUMvxl:
  case VE::veoldPVFMKSNANvl: case VE::veoldPVFMKSNANvxl:
  case VE::veoldPVFMKSGTNANvl: case VE::veoldPVFMKSGTNANvxl:
  case VE::veoldPVFMKSLTNANvl: case VE::veoldPVFMKSLTNANvxl:
  case VE::veoldPVFMKSNENANvl: case VE::veoldPVFMKSNENANvxl:
  case VE::veoldPVFMKSEQNANvl: case VE::veoldPVFMKSEQNANvxl:
  case VE::veoldPVFMKSGENANvl: case VE::veoldPVFMKSGENANvxl:
  case VE::veoldPVFMKSLENANvl: case VE::veoldPVFMKSLENANvxl: {
    expandPseudoVFMK_VL(*this, MI);
    return true;
  }
#endif
  }
  return false;
}

bool VEInstrInfo::expandExtendStackPseudo(MachineInstr &MI) const {
  MachineBasicBlock &MBB = *MI.getParent();
  MachineFunction &MF = *MBB.getParent();
  const VESubtarget &STI = MF.getSubtarget<VESubtarget>();
  const VEInstrInfo &TII = *STI.getInstrInfo();
  DebugLoc dl = MBB.findDebugLoc(MI);

  // Create following instructions and multiple basic blocks.
  //
  // thisBB:
  //   brge.l.t %sp, %sl, sinkBB
  // syscallBB:
  //   ld      %s61, 0x18(, %tp)        // load param area
  //   or      %s62, 0, %s0             // spill the value of %s0
  //   lea     %s63, 0x13b              // syscall # of grow
  //   shm.l   %s63, 0x0(%s61)          // store syscall # at addr:0
  //   shm.l   %sl, 0x8(%s61)           // store old limit at addr:8
  //   shm.l   %sp, 0x10(%s61)          // store new limit at addr:16
  //   monc                             // call monitor
  //   or      %s0, 0, %s62             // restore the value of %s0
  // sinkBB:

  // Create new MBB
  MachineBasicBlock *BB = &MBB;
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineBasicBlock *syscallMBB = MF.CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *sinkMBB = MF.CreateMachineBasicBlock(LLVM_BB);
  MachineFunction::iterator It = ++(BB->getIterator());
  MF.insert(It, syscallMBB);
  MF.insert(It, sinkMBB);

  // Transfer the remainder of BB and its successor edges to sinkMBB.
  sinkMBB->splice(sinkMBB->begin(), BB,
                  std::next(std::next(MachineBasicBlock::iterator(MI))),
                  BB->end());
  sinkMBB->transferSuccessorsAndUpdatePHIs(BB);

  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(syscallMBB);
  BB->addSuccessor(sinkMBB);
  BuildMI(BB, dl, TII.get(VE::BRCFLrr_t))
      .addImm(VECC::CC_IGE)
      .addReg(VE::SX11) // %sp
      .addReg(VE::SX8)  // %sl
      .addMBB(sinkMBB);

  BB = syscallMBB;

  // Update machine-CFG edges
  BB->addSuccessor(sinkMBB);

  BuildMI(BB, dl, TII.get(VE::LDrii), VE::SX61)
      .addReg(VE::SX14)
      .addImm(0)
      .addImm(0x18);
  BuildMI(BB, dl, TII.get(VE::ORri), VE::SX62)
      .addReg(VE::SX0)
      .addImm(0);
  BuildMI(BB, dl, TII.get(VE::LEAzii), VE::SX63)
      .addImm(0)
      .addImm(0)
      .addImm(0x13b);
  BuildMI(BB, dl, TII.get(VE::SHMLri))
      .addReg(VE::SX61)
      .addImm(0)
      .addReg(VE::SX63);
  BuildMI(BB, dl, TII.get(VE::SHMLri))
      .addReg(VE::SX61)
      .addImm(8)
      .addReg(VE::SX8);
  BuildMI(BB, dl, TII.get(VE::SHMLri))
      .addReg(VE::SX61)
      .addImm(16)
      .addReg(VE::SX11);
  BuildMI(BB, dl, TII.get(VE::MONC));

  BuildMI(BB, dl, TII.get(VE::ORri), VE::SX0)
      .addReg(VE::SX62)
      .addImm(0);

  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool VEInstrInfo::expandGetStackTopPseudo(MachineInstr &MI) const {
  MachineBasicBlock *MBB = MI.getParent();
  MachineFunction &MF = *MBB->getParent();
  const VESubtarget &STI = MF.getSubtarget<VESubtarget>();
  const VEInstrInfo &TII = *STI.getInstrInfo();
  DebugLoc DL = MBB->findDebugLoc(MI);

  // Create following instruction
  //
  //   dst = %sp + target specific frame + the size of parameter area

  const MachineFrameInfo &MFI = MF.getFrameInfo();
  const VEFrameLowering &TFL = *STI.getFrameLowering();

  // The VE ABI requires a reserved 176 bytes area at the top
  // of stack as described in VESubtarget.cpp.  So, we adjust it here.
  unsigned NumBytes = STI.getAdjustedFrameSize(0);

  // Also adds the size of parameter area.
  if (MFI.adjustsStack() && TFL.hasReservedCallFrame(MF))
    NumBytes += MFI.getMaxCallFrameSize();

  BuildMI(*MBB, MI, DL, TII.get(VE::LEArii))
      .addDef(MI.getOperand(0).getReg())
      .addReg(VE::SX11)
      .addImm(0)
      .addImm(NumBytes);

  MI.eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}
