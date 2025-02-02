//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "sisfsb.h"
#include "chips.h"
#include "pci.h"

bool SiSFSB::run() {
  const std::string PLLName = Args.PLL;
  if (PLLName.empty() || !AllChips.supportPLL(PLLName)) {
    if (!PLLName.empty())
      std::cerr << "Unsupported PLL: '" << PLLName << "'" << std::endl;
    std::cerr << "Please specify a supported PLL: -pll <PLL>" << std::endl;
    AllChips.listSupportedPLLs(std::cerr);
    return false;
  }
  PLL *Pll = AllChips.findPLL(Args.PLL);
  if (Pll == nullptr)
    exit(1);

  const std::string &FreqStr = Args.Fsb.getFreqStr();
  if (toLower(FreqStr) == FreqEntry::ListStr) {
    Pll->dumpFreqTable(std::cout);
    return true;
  }

  std::cout << std::hex;
  std::cerr << std::hex;

  // Find a supported host bridge based on VendorID/DeviceID.
  HostToPCIBridge *HostBridge = AllChips.findHostBridge();
  if (HostBridge == nullptr)
    exit(1);

  // Initialize the I2C (SMB) bus, which is where the PLL lives.
  if (!HostBridge->initSMB()) {
    std::cerr << "Failed to initialize SMB" << std::endl;
    exit(1);
  }
  std::cout << "SMB initialized successfully" << std::endl;
  auto &SMB = HostBridge->getSMB();
  std::cout << SMB << std::endl;

  // Do a quick write check to the PLL.
  if (!Pll->check(*HostBridge))
    exit(1);
  std::cout << "PLL Chip passed quick write check: " << *Pll << std::endl;

  // Query PLL for FSB via the I2C Bus (SMBus).
  std::optional<FreqEntry> FEOpt = Pll->getFSB(SMB);
  if (!FEOpt)
    exit(1);
  std::cout << "Current FSB: " << *FEOpt << std::endl;

  if (Args.Fsb.bad())
    exit(0);

  // Try to set the new FSB.
  std::cout << "Setting new FSB: " << Args.Fsb << std::endl;
  if (!Pll->setFSB(Args.Fsb, SMB)) {
    std::cerr << "Error setting FSB: " << *FEOpt << std::endl;
    exit(1);
  }
  // Get the FSB once again to check if it was set.
  FEOpt = Pll->getFSB(SMB);
  if (!FEOpt)
    exit(1);
  std::cout << "Current FSB: " << *FEOpt << std::endl;
  return true;
}
