//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//
// This is where we register new SouthBridge and PLL chips.
//

#ifndef __SRC_CHIPS_H__
#define __SRC_CHIPS_H__

#include "freqentry.h"
#include "pci.h"
#include "smbus.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// A simple base class for everything with a name.
class Named {
protected:
  std::string Name;

public:
  Named(const std::string &Name) : Name(Name) {}
  const std::string &getName() const { return Name; }
};

/// A helper class for VendorID and DeviceID.
struct VendorDeviceID {
  uint16_t VendorID;
  uint16_t DeviceID;
  VendorDeviceID(uint16_t VendorID, uint16_t DeviceID)
      : VendorID(VendorID), DeviceID(DeviceID) {}
  bool operator==(const VendorDeviceID &Other) const {
    return VendorID == Other.VendorID && DeviceID == Other.DeviceID;
  }
  bool operator!=(const VendorDeviceID &Other) const {
    return !(*this == Other);
  }
  friend std::ostream &operator<<(std::ostream &OS, const VendorDeviceID &ID) {
    OS << ID.VendorID << ":" << ID.DeviceID;
    return OS;
  }
};
/// Helper hasher for VendorDeviceID.
struct VendorDeviceIDHasher {
  uint32_t operator()(const VendorDeviceID &ID) const {
    return ((uint32_t)ID.VendorID) << 16 | (uint32_t)ID.DeviceID;
  }
};

/// A base class for function blocks of the PCI bus.
class FunctionBlock : public Named {
protected:
  BDF BDFAddr;
  VendorDeviceID ID;

public:
  FunctionBlock(const std::string &Name, uint16_t VendorID, uint16_t DeviceID,
                const BDF &BDFAddr)
      : Named(Name), BDFAddr(BDFAddr), ID(VendorID, DeviceID) {}
  uint16_t getVendorID() const { return ID.VendorID; }
  uint16_t getDeviceID() const { return ID.DeviceID; }
  const VendorDeviceID &getVendorDeviceID() const { return ID; }
  const BDF &getBDF() const { return BDFAddr; }
  virtual void print(std::ostream &OS) const {
    OS << Name << " (" << ID << ")";
  }
  friend std::ostream &operator<<(std::ostream &OS, const FunctionBlock &FB) {
    FB.print(OS);
    return OS;
  }
};


class HostToPCIBridge : public FunctionBlock {
protected:
  /// The SMBus base register within the ACPI address space.
  uint16_t SMBusBaseReg = 0;

  /// Do any necessary initializations (like turning ON ACPI) and return the
  /// base address where the SMBus can be found.
  virtual std::optional<uint16_t> getSMBusAddr() const = 0;

  std::unique_ptr<SMBus> SMB;

  HostToPCIBridge(const std::string &Name, uint16_t VendorID, uint16_t DeviceID,
                  const BDF &BDFAddr, uint16_t SMBusBaseReg)
      : FunctionBlock(Name, VendorID, DeviceID, BDFAddr),
        SMBusBaseReg(SMBusBaseReg) {}

public:
  /// Find the SMBus address and create the SMB object.
  virtual bool initSMB() = 0;

  SMBus &getSMB() { return *SMB; }
  void printHB(std::ostream &OS) const {
    OS << "SMBus:" << (int)SMBusBaseReg;
  }
};

class SiS540 : public HostToPCIBridge {
  /// The LPC holds the ACPI BaseAddress.
  FunctionBlock LPC;

  /// The LPC registers and masks.
  static constexpr const uint16_t LPC_BiosCtrlReg = 0x40;
  static constexpr const uint16_t LPC_EnableACPIMask = 0x80;
  static constexpr const uint16_t LPC_ACPIBaseAddrReg = 0x74;

  std::optional<uint16_t> getSMBusAddr() const override;

public:
  SiS540()
      : HostToPCIBridge("SiS540", /*VendorID=*/0x1039, /*DeviceID=*/0x0540,
                        BDF(0, 0, 0),
                        /*SMBusBaseReg=*/0x80),
        LPC("SiSLPC", getVendorID(), /*DeviceID=*/0x0008, BDF(0, 1, 8)) {}

  bool initSMB() override;
  void print(std::ostream &OS) const override {
    static_cast<FunctionBlock>(*this).print(OS);
    OS << " ";
    printHB(OS);
  }
};

class PLL : public Named {
protected:
  /// The PLL register holding the key bits (usually 0).
  unsigned KeyRegister;
  /// The bit number of each of the bits making up the key, starting from 0.
  /// For example if KEY0 bit is bit 4, KEY1 bit 5, KEY2 bit 6 and KEY3 bit 2,
  /// then this should be {4, 5, 6, 2}.
  std::vector<uint8_t> KeyBits;
  /// Maps the key to the frequency entry. For example:
  /// Key 0000 to 100MHz FSB, 100MHz SDRam, 33MHz PCI
  /// {0b0000, {100, 100, 33}}
  std::map<uint8_t, FreqEntry> FreqTable;

  /// The register for enabling the I2C operation of the PLL
  unsigned EnableI2CRegister = 0;
  /// The bit in the `EnableI2CRegister` for enabling the I2C operation.
  uint8_t EnableI2CBit = 0;

  /// \Returns the key value given the value of the KeyRegister.
  uint8_t getKey(uint8_t KeyRegVal) const;
  /// Look up entry \p FE in the frequency table and return the key.
  std::optional<uint8_t> lookupKey(const FreqEntry &FE) const;
  /// \Returns the updated key register by encoding \p Key into it.
  uint8_t encodeKey(uint8_t OrigKeyReg, uint8_t Key) const;

public:
  void dumpFreqTable(std::ostream &OS) const;
  /// The PLL address in the SMBus according to the datasheet.
  static constexpr const uint8_t SlaveAddr = 0x69;
  /// The magic CMD we need to send according to the datasheet.
  static constexpr const uint8_t Cmd = 0x0;

  PLL(const std::string &Name, unsigned KeyRegister,
      const std::vector<uint8_t> &KeyBits,
      const std::map<uint8_t, FreqEntry> &FreqTable, unsigned EnableI2CRegister,
      uint8_t EnableI2CBit)
      : Named(Name), KeyRegister(KeyRegister), KeyBits(KeyBits),
        FreqTable(FreqTable), EnableI2CRegister(EnableI2CRegister),
        EnableI2CBit(EnableI2CBit) {}

  // Can be overriden for chip-specific implementations.
  virtual std::optional<FreqEntry> getFSB(SMBus &SMB) const;

  // Can be overriden for chip-specific implementations.
  virtual bool setFSB(const FreqEntry &FE, SMBus &SMB);

  // Can be overriden for chip-specific implementations.
  virtual bool getEnabled(SMBus &SMB) const;

  // Can be overriden for chip-specific implementations.
  virtual void setEnabled(bool NewVal, SMBus &SMB) const;

  // Check the PLL with a quick write.
  bool check(HostToPCIBridge &HB) const;

  friend std::ostream &operator<<(std::ostream &OS, const PLL &P) {
    OS << P.getName();
    return OS;
  }
};

class Chips {
  std::vector<PLL> PLLs;

  /// The Host-to-pci Bridge chips we support.
  std::vector<std::unique_ptr<HostToPCIBridge>> HostBridges;

  /// This is where all chips get defined (in the .cpp file).
  void registerChips();

public:
  Chips() { registerChips(); }

  void listHostBridges(std::ostream &OS) const;

  /// Searches for any of the registered host bridges. \Returns null if not
  /// found.
  HostToPCIBridge *findHostBridge() const;

  /// Use the HostBridge \p HB to look if PLL with name \p PLLName exists.
  PLL *findPLL(const std::string &PLLName);

  /// \Returns true if we support a PLL named \p PLLName.
  bool supportPLL(const std::string &PLLName);

  /// Lists all supported PLLs.
  void listSupportedPLLs(std::ostream &OS) const;
};

#endif // __SRC_CHIPS_H__
