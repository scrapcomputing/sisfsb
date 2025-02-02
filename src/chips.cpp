//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//
// This is where we register new SouthBridge and PLL chips.
//

#include "chips.h"
#include "utils.h"
#include <memory>

std::optional<uint16_t> SiS540::getSMBusAddr() const {
  // The SMBus address is found in the LPC function block.
  // So first check if LPC responds.
  uint16_t FoundVendorID = PCI::readWord(LPC.getBDF(), PCI::VendorIdReg);
  if (FoundVendorID != LPC.getVendorID()) {
    std::cerr << "Found LPC Vendor ID " << FoundVendorID << " expected "
              << getVendorID() << std::endl;
    return std::nullopt;
  }
  uint16_t FoundDeviceID = PCI::readWord(LPC.getBDF(), PCI::DeviceIdReg);
  if (FoundDeviceID != LPC.getDeviceID()) {
    std::cerr << "Found LPC Device ID " << FoundDeviceID << " expected "
              << LPC.getDeviceID() << std::endl;
    return std::nullopt;
  }
  // OK so we can now access LPC.
  // Enable ACPI by setting a bit in the BiosCtrlReg of LPC.
  uint8_t BCR = PCI::readByte(LPC.getBDF(), LPC_BiosCtrlReg);
  if (Debug)
    std::cout << "BiosCtrlReg=0x" << BCR << std::endl;
  if (!(BCR & LPC_EnableACPIMask)) {
    PCI::writeByte(LPC.getBDF(), LPC_BiosCtrlReg, BCR | LPC_EnableACPIMask);
    BCR = PCI::readByte(LPC.getBDF(), LPC_BiosCtrlReg);
  }
  // Early return if we could not enable ACPI.
  if (!(BCR & LPC_EnableACPIMask)) {
    std::cerr << "Could not enable ACPI!" << std::endl;
    return std::nullopt;
  }
  // The SMBus address is in LPC at LPC_ACPIBaseAddrReg.
  uint16_t Addr = PCI::readWord(LPC.getBDF(), LPC_ACPIBaseAddrReg);
  if (Addr == 0xffff || Addr == 0) {
    std::cerr << "Found bad SMBus address in LPC at "
              << (int)LPC_ACPIBaseAddrReg << std::endl;
    return std::nullopt;
  }
  std::cout << "Found SMBus addr: " << Addr << "\n";
  return Addr;
}

bool SiS540::initSMB() {
  std::optional<uint16_t> SMBAddr = getSMBusAddr();
  if (! SMBAddr)
    return false;
  SMB = std::make_unique<SiSSMBus>(*SMBAddr, PLL::SlaveAddr);
  return true;
}

void PLL::dumpFreqTable(std::ostream &OS) const {
  OS << "FreqTable FSB/SDRAM/PCI   Divider" << std::endl;
  for (auto [Key, FE] : FreqTable)
    OS << std::setw(2) << (int)Key << "    : " << FE
       << std::endl;
}

uint8_t PLL::getKey(uint8_t KeyRegVal) const {
  uint8_t Key = 0;
  for (unsigned Cnt = 0, E = KeyBits.size(); Cnt != E; ++Cnt) {
    unsigned KeyBit = KeyBits[Cnt];
    bool BitIsSet = KeyRegVal & (0x1 << KeyBit);
    bool Mask = BitIsSet ? 0x1 : 0x0;
    Key |= Mask << Cnt;
  }
  return Key;
}

std::optional<uint8_t> PLL::lookupKey(const FreqEntry &FE) const {
  for (auto &[Key, TableFE] : FreqTable) {
    if (TableFE == FE)
      return Key;
  }
  return std::nullopt;
}

uint8_t PLL::encodeKey(uint8_t OrigKeyReg, uint8_t Key) const {
  uint8_t NewKeyReg = OrigKeyReg;
  for (int Bit = 0; Bit != 8; ++Bit) {
    bool IsOne = Key & 0x1 << Bit;
    int Shift = KeyBits[Bit];
    if (IsOne)
      NewKeyReg |= 0x1 << Shift;
    else
      NewKeyReg &= ~(0x1 << Shift);
  }
  return NewKeyReg;
}

std::optional<FreqEntry> PLL::getFSB(SMBus &SMB) const {
  auto ReadVec = SMB.readBlockData(KeyRegister, Cmd);
  if (ReadVec.empty()) {
    std::cerr << "Could not read block from PLL (Reg=0x" << KeyRegister << ")"
              << std::endl;
    return std::nullopt;
  }
  if (Debug) {
    std::cout << "ReadVec: ";
    for (auto Byte : ReadVec)
      std::cout << "0x" << (int)Byte << " ";
    std::cout << std::endl;
  }

  uint8_t Reg = ReadVec[0];
  uint8_t Key = getKey(Reg);
  if (Debug) {
    DecimalGuard(std::cout);
    std::cout << "PLL Reg=" << (int)Reg << " Key=" << (int)Key << std::endl;
  }
  auto It = FreqTable.find(Key);
  if (It == FreqTable.end()) {
    if (Debug) {
      DecimalGuard(std::cerr);
      std::cerr << "Key " << (int)Key << " not found in FreqTable" << std::endl;
    }
    dumpFreqTable(std::cerr);
    return std::nullopt;
  }
  return It->second;
}

bool PLL::setFSB(const FreqEntry &FE, SMBus &SMB) {
  std::optional<uint8_t> KeyOpt = lookupKey(FE);
  if (!KeyOpt) {
    std::cerr << "Could not find " << FE << " in FreqTable." << std::endl;
    dumpFreqTable(std::cerr);
    return false;
  }
  uint8_t Key = *KeyOpt;
  if (Debug)
    std::cout << "PLL Key = 0x" << (int)Key << std::endl;
  auto OldKeyVec = SMB.readBlockData(KeyRegister, Cmd);
  if (OldKeyVec.empty()) {
    std::cerr << "Could not read original Key Reg." << std::endl;
    return false;
  }
  if (Debug)
    std::cout << "PLL OldKeyVec[0] = 0x" << (int)OldKeyVec[0] << std::endl;

  uint8_t NewKeyReg = encodeKey(OldKeyVec[0], Key);
  std::cout << "PLL NewKeyReg = 0x" << (int)NewKeyReg << std::endl;
  std::vector<uint8_t> Data = {NewKeyReg};
  if (!SMB.writeBlockData(KeyRegister, Cmd, Data)) {
    std::cerr << "Failed to write block data to PLL" << std::endl;
    return false;
  }

  if (Debug)
    std::cout << "PLL Enabling I2C" << std::endl;
  setEnabled(true, SMB);
  return true;
}

static uint8_t getReg(unsigned Reg, SMBus &SMB) {
  if (Debug)
    std::cout << __FUNCTION__ << "(" << Reg << ")" << std::endl;
  auto RegVec = SMB.readBlockData(Reg, PLL::Cmd);
  if (RegVec.empty()) {
    std::cerr << __FUNCTION__ << " Failed to get the enabled bit." << std::endl;
    exit(1);
  }
  uint8_t RegVal = RegVec[0];
  if (Debug)
    std::cout << __FUNCTION__ << " Reg = 0x" << (int)RegVal << std::endl;
  return RegVal;
}

bool PLL::getEnabled(SMBus &SMB) const {
  uint8_t EnableI2CMask = (uint8_t)0x1 << EnableI2CBit;
  bool Enabled = getReg(EnableI2CRegister, SMB) & EnableI2CMask;
  if (Debug)
    std::cout << "PLL Enabled = " << Enabled << std::endl;
  return Enabled;
}

void PLL::setEnabled(bool NewVal, SMBus &SMB) const {
  if (Debug)
    std::cout << "PLL setEnabled(" << NewVal << ")" << std::endl;
  uint8_t OldReg = getReg(EnableI2CRegister, SMB);
  uint8_t EnableI2CMask = (uint8_t)0x1 << EnableI2CBit;
  uint8_t NewReg = OldReg | EnableI2CMask;
  if (Debug)
    std::cout << "PLL NewReg = 0x" << (int)NewReg << std::endl;
  bool Success = SMB.writeBlockData(EnableI2CRegister, Cmd, {NewReg});
  if (!Success) {
    std::cerr << "PLL Failed to set the enabled bit." << std::endl;
    exit(1);
  }
}

bool PLL::check(HostToPCIBridge &HB) const {
  SMBus &SMB = HB.getSMB();
  if (!SMB.writeQuick(0)) {
    std::cerr << "PLL Failed writeQuick()!" << std::endl;
    return false;
  }
  return true;
}

void Chips::registerChips() {
  // Register Host-to-pci bridges.
  HostBridges.push_back(std::make_unique<SiS540>());

  // Register PLLs.
  int Key = 0;
  PLLs.push_back(PLL(
      /*Name=*/"W83194R-630A",
      /*KeyRegister=*/0,
      /*KeyBits=*/{4, 5, 6, 2},
      {
          {Key++, {66.80, 100.20, 33.4}},
          {Key++, {100.20, 100.20, 33.4}},
          {Key++, {83.30, 83.30, 33.2}},
          {Key++, {133.60, 100.20, 33.4}},
          {Key++, {75.00, 75.00, 37.5}},
          {Key++, {100.20, 133.60, 33.4}},
          {Key++, {100.20, 150.30, 33.4}},
          {Key++, {133.60, 133.60, 33.4}},
          {Key++, {66.80, 66.80, 33.4}},
          {Key++, {97.00, 97.00, 32.3}},
          {Key++, {97.00, 129.30, 32.3}},
          {Key++, {95.20, 95.20, 31.7}},
          {Key++, {140.00, 140.00, 35.0}},
          {Key++, {112.00, 112.00, 37.3}},
          {Key++, {96.20, 96.20, 32.1}},
          {Key++, {166.00, 166.00, 33.3}},
      },
      /*EnableI2CRegister=*/0,
      /*EnableI2cBit=*/3));
}

void Chips::listHostBridges(std::ostream &OS) const {
  OS << "Supported Host Bridges:" << std::endl;
  for (const auto &HBPtr : HostBridges) {
    OS << *HBPtr << std::endl;
  }
}

HostToPCIBridge *Chips::findHostBridge() const {
  HostToPCIBridge *HB = nullptr;
  for (const auto &HBPtr : HostBridges) {
    const BDF &BDF = HBPtr->getBDF();
    uint16_t VendorID = PCI::readWord(BDF, PCI::VendorIdReg);
    if (HBPtr->getVendorID() != VendorID)
      continue;
    uint16_t DeviceID = PCI::readWord(BDF, PCI::DeviceIdReg);
    if (HBPtr->getDeviceID() != DeviceID)
      continue;
    HB = HBPtr.get();
    break;
  }
  if (HB == nullptr) {
    std::cerr << "Failed to find a supported Host Bridge!" << std::endl;
    listHostBridges(std::cerr);
    return nullptr;
  }
  std::cout << "Host bridge found: " << *HB << std::endl;
  return HB;
}

PLL *Chips::findPLL(const std::string &PLLName) {
  auto It = std::find_if(PLLs.begin(), PLLs.end(), [&PLLName](const PLL &P) {
    return toLower(P.getName()) == toLower(PLLName);
  });
  if (It == PLLs.end()) {
    std::cerr << "PLL '" << PLLName << "' not supported!" << std::endl;
    return nullptr;
  }
  return &*It;
}

bool Chips::supportPLL(const std::string &PLLName) {
  return findPLL(PLLName) != nullptr;
}

void Chips::listSupportedPLLs(std::ostream &OS) const {
  OS << "Supported PLLs:" << std::endl;
  for (const auto &PLL : PLLs) {
    OS << "  " << PLL.getName() << std::endl;
  }
}
