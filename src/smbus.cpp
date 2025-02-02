//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "smbus.h"
#include <iostream>

bool SiSSMBus::transfer(TransferTy TrTy) {
  if (Debug) {
    DecimalGuard DG(std::cout);
    std::cout << "SMBus " << __FUNCTION__
              << "(TrTy=" << (int)getTransferTyMask(TrTy) << ")" << std::endl;
  }
  // Check if ready.
  uint8_t Control = getControl();
  uint32_t Timeout = 10;
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__
              << " Checking if HostBusyMask|SlaveBusyMask";
  if (Control & (HostBusyMask | SlaveBusyMask)) {
    // Try to kill the current transfer.
    setHostControl(KillMask, TransferTy::Quick);
    delay(100);
    Control = getControl();
    if (Debug)
      std::cout << " .. ";
  }
  while (Control & (HostBusyMask | SlaveBusyMask)) {
    Control = getControl();
    delay(100);
    if (--Timeout == 0) {
      std::cerr << "Host or slave busy!" << std::endl;
      return false;
    }
    if (Debug)
      std::cout << ".";
  }
  if (Debug)
    std::cout << "Done!" << std::endl;

  // TODO: Is this needed?
  // Disable timeout interrupt
  setControl(Control & ~HostMasterTimeoutMask);
  // TODO: Is this needed?
  setStatus(getStatus() & ClearSlaveAlertSlaveAliasHostSlaveMask);

  auto WaitForTransfer = [this, TrTy]() {
    if (Debug)
      std::cout << "WaitForTransfer ";
    uint32_t TimeoutCnt = 0;
    uint8_t Status = 0;
    do {
      // Sleep 100ms otherwise SMBus will be busy
      delay(100);
      if (Debug)
        std::cout << ".";
      Status = getStatus();
      if (TrTy == TransferTy::BlockData && (Status & BlockFinishedMask))
        break;
    } while ((Status & ErrMask) && (++TimeoutCnt < TransferTimeout));
    if (TimeoutCnt >= TransferTimeout) {
      std::cerr << "Transfer timeout" << std::endl;
      return false;
    }
    if (Status & ErrMask) {
      std::cerr << "Transfer failed (error)" << std::endl;
      return false;
    }
    if (Debug)
      std::cout << "Done" << std::endl;
    return true;
  };
  if (!WaitForTransfer()) {
    std::cerr << "Transfer timeout" << std::endl;
    return false;
  }

  // Start transfer by setting bit 4.
  setHostControl(StartTransferMask, TrTy);

  if (!WaitForTransfer()) {
    std::cerr << "Transfer timeout" << std::endl;
    return false;
  }

  // End transaction, clear sticky bits
  setStatus(ClearStickyBitsMask);
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << " Finished!" << std::endl;
  return true;
}

bool SiSSMBus::readQuick(uint8_t Addr) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr << ")"
              << std::endl;
  setAddr(SlaveAddr, RW::Read);
  return transfer(TransferTy::Quick);
}

bool SiSSMBus::writeQuick(uint8_t Addr) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr << ")"
              << std::endl;
  setAddr(SlaveAddr, RW::Write);
  return transfer(TransferTy::Quick);
}

std::optional<uint8_t> SiSSMBus::readByte(uint8_t Addr) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr << ")"
              << std::endl;
  setAddr(SlaveAddr, RW::Read);
  return transfer(TransferTy::Byte);
}

bool SiSSMBus::writeByte(uint8_t Addr, uint8_t Cmd) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr
              << ", Cmd=" << (int)Cmd << ")" << std::endl;
  setCmd(Cmd);
  setAddr(SlaveAddr, RW::Write);
  return transfer(TransferTy::Byte);
}

std::optional<uint8_t> SiSSMBus::readByteData(uint8_t Addr, uint8_t Cmd) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr
              << ", Cmd=" << (int)Cmd << ")" << std::endl;
  setCmd(Cmd);
  setAddr(SlaveAddr, RW::Read);
  if (!transfer(TransferTy::ByteData))
    return std::nullopt;
  return getData(/*Offset=*/0);
}

bool SiSSMBus::writeByteData(uint8_t Addr, uint8_t Cmd, uint8_t Val) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr
              << ", Cmd=" << (int)Cmd << ", Val=" << (int)Val << ")"
              << std::endl;
  setCmd(Cmd);
  setLen(Val);
  setAddr(SlaveAddr, RW::Write);
  return transfer(TransferTy::ByteData);
}

std::vector<uint8_t> SiSSMBus::readBlockData(uint8_t Addr, uint8_t Cmd) {
  if (Debug)
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr
              << ", Cmd=0x" << (int)Cmd << ")" << std::endl;
  setCmd(Cmd);
  setAddr(SlaveAddr, RW::Read);
  if (!transfer(TransferTy::BlockData))
    return {};
  uint8_t Len = std::min(getLen(), (uint8_t)32);

  std::vector<uint8_t> RetVec;
  for (uint8_t Cnt = 0; Cnt != Len; ++Cnt) {
    uint8_t Offset = Cnt % 8;
    RetVec.push_back(getData(Offset));
    if (Offset == 7)
      setStatus(BlockFinishedMask);
  }
  return RetVec;
}

bool SiSSMBus::writeBlockData(uint8_t Addr, uint8_t Cmd,
                              const std::vector<uint8_t> Data) {
  if (Debug) {
    std::cout << "SMBus " << __FUNCTION__ << "(Addr=0x" << (int)Addr
              << ", Cmd=" << (int)Cmd << ", Data=[";
    for (auto D : Data)
      std::cout << (int)D << ", ";
    std::cout << "])" << std::endl;
  }
  setCmd(Cmd);
  for (uint8_t Offset = 0, E = Data.size(); Offset != E; ++Offset)
    setData(Data[Offset], Offset);
  setAddr(SlaveAddr, RW::Write);
  return transfer(TransferTy::BlockData);
}

void SiSSMBus::print(std::ostream &OS) const {
  OS << Name << " BaseAddr: 0x" << (int)BaseAddr << " SlaveAddr: 0x"
     << (int)SlaveAddr << std::endl;
}
