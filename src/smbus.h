//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __SRC_SMBUS_H__
#define __SRC_SMBUS_H__

#include "utils.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

class SMBus {
protected:
  std::string Name;
  /// The base address of SMBus.
  const uint16_t BaseAddr;
  /// The address of the SMB component we want to talk to.
  uint16_t SlaveAddr;

  SMBus(std::string Name, uint16_t BaseAddr, uint16_t SlaveAddr)
      : Name(Name), BaseAddr(BaseAddr), SlaveAddr(SlaveAddr) {}

public:
  virtual bool readQuick(uint8_t Addr) = 0;
  virtual bool writeQuick(uint8_t Addr) = 0;
  virtual std::optional<uint8_t> readByte(uint8_t Addr) = 0;
  virtual bool writeByte(uint8_t Addr, uint8_t Cmd) = 0;
  virtual std::optional<uint8_t> readByteData(uint8_t Addr, uint8_t Cmd) = 0;
  virtual bool writeByteData(uint8_t Addr, uint8_t Cmd, uint8_t Val) = 0;
  virtual std::vector<uint8_t> readBlockData(uint8_t Addr, uint8_t Cmd) = 0;
  virtual bool writeBlockData(uint8_t Addr, uint8_t Cmd,
                              const std::vector<uint8_t> Data) = 0;
  virtual void print(std::ostream &OS) const = 0;
  friend std::ostream &operator<<(std::ostream &OS, const SMBus &SMB) {
    SMB.print(OS);
    return OS;
  }
};

class SiSSMBus final : public SMBus {
  // Control registers
  static constexpr const uint8_t SMB_STS = 0x80;
  static constexpr const uint8_t SMB_EN = 0x81;
  static constexpr const uint8_t SMB_CNT = 0x82;
  static constexpr const uint8_t SMB_HOST_CNT = 0x83;
  static constexpr const uint8_t SMB_ADDR = 0x84;
  static constexpr const uint8_t SMB_CMD = 0x85;
  static constexpr const uint8_t SMB_PCOUNT = 0x86;
  static constexpr const uint8_t SMB_COUNT = 0x87;
  static constexpr const uint8_t SMB_BYTE0_7 = 0x88;
  static constexpr const uint8_t SMBDEV_ADDR = 0x90;
  static constexpr const uint8_t SMB_DB0 = 0x91;
  static constexpr const uint8_t SMB_DB1 = 0x92;
  static constexpr const uint8_t SMB_SAA = 0x93;

  // Masks for SMB_CNT
  static constexpr const uint8_t HostBusyMask = 0x01;
  static constexpr const uint8_t SlaveBusyMask = 0x02;
  static constexpr const uint8_t HostMasterTimeoutMask = 0x40;

  // Masks for SMB_HOST_CNT
  static constexpr const uint8_t StartTransferMask = 0x10; // bit 4
  static constexpr const uint8_t KillMask = 0x20; // bit 5
  static constexpr const uint8_t QuickCommand = 0b000;

  enum class TransferTy : uint8_t {
    Quick = 0b000,
    Byte = 0b001,
    ByteData = 0b010,
    WordData = 0b011,
    BlockData = 0b101,
  };

  static uint8_t getTransferTyMask(TransferTy Ty) { return (uint8_t)Ty; }

  // Masks for SMB_STS
  static constexpr const uint8_t DevErrMask = 0x02;
  static constexpr const uint8_t CollisionMask = 0x04;
  static constexpr const uint8_t TrCompleteMask = 0x08;
  static constexpr const uint8_t ErrMask = DevErrMask | CollisionMask;
  static constexpr const uint8_t BlockFinishedMask = 0x10;
  static constexpr const uint8_t ClearStickyBitsMask = 0xff;
  // Clear bits 7,6,5,0 slave_alert, slave_alias_addr, host_slave,
  static constexpr const uint8_t ClearSlaveAlertSlaveAliasHostSlaveMask = 0x1e;

  static constexpr const uint32_t TransferTimeout = 40;

  // Bit masks
  static constexpr const uint8_t ReadMask = 0x01;
  static constexpr const uint8_t WriteMask = 0x00;


  bool transfer(TransferTy TrTy);

  enum class RW {
    Read,
    Write,
  };
  void setAddr(uint8_t Addr, RW ReadOrWrite) {
    uint8_t RWMask = ReadOrWrite == RW::Read ? ReadMask : WriteMask;
    if (Debug)
      std::cout << "SMBus " << __FUNCTION__ << "(addr=0x" << BaseAddr << "+0x"
                << (int)SMB_ADDR << ", val=0x" << (int)Addr << ")" << std::endl;
    outportb(BaseAddr + SMB_ADDR, ((Addr & 0x7f) << 1) | RWMask);
  }
  void setCmd(uint8_t Cmd) { outportb(BaseAddr + SMB_CMD, Cmd); }

  void setLen(uint8_t Len) { outportb(BaseAddr + SMB_COUNT, Len); }

  uint8_t getLen() { return inportb(BaseAddr + SMB_COUNT); }

  uint8_t getData(uint8_t Offset) {
    return inportb(BaseAddr + SMB_BYTE0_7 + Offset);
  }
  void setData(uint8_t Byte, uint8_t Offset) {
    outportb(BaseAddr + SMB_BYTE0_7 + Offset, Byte);
  }

  uint8_t getControl() { return inportb(BaseAddr + SMB_CNT); }

  void setControl(uint8_t Val) { outportb(BaseAddr + SMB_CNT, Val); }

  void setHostControl(uint8_t Val, TransferTy Ty) {
    uint8_t TyMask = getTransferTyMask(Ty);
    outportb(BaseAddr + SMB_HOST_CNT, Val | TyMask);
  }

  uint8_t getStatus() { return inportb(BaseAddr + SMB_STS); }

  void setStatus(uint8_t Val) { outportb(BaseAddr + SMB_STS, Val); }

public:
  SiSSMBus(uint16_t BaseAddr, uint16_t SlaveAddr)
      : SMBus("SiSSMBus", BaseAddr, SlaveAddr) {}

  bool readQuick(uint8_t Addr) override;
  bool writeQuick(uint8_t Addr) override;
  std::optional<uint8_t> readByte(uint8_t Addr) override;
  bool writeByte(uint8_t Addr, uint8_t Cmd) override;
  std::optional<uint8_t> readByteData(uint8_t Addr, uint8_t Cmd) override;
  bool writeByteData(uint8_t Addr, uint8_t Cmd, uint8_t Val) override;
  std::vector<uint8_t> readBlockData(uint8_t Addr, uint8_t Cmd) override;
  bool writeBlockData(uint8_t Addr, uint8_t Cmd,
                      const std::vector<uint8_t> Data) override;
  void print(std::ostream &OS) const override;
};

#endif // __SRC_SMBUS_H__
