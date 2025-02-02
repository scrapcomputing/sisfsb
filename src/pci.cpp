//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "pci.h"

void PCI::forEachBDF(std::function<bool(const BDF &)> Fn) {
  for (uint16_t Bus = BDF::BusMin; Bus != BDF::BusMax; ++Bus) {
    for (uint16_t Dev = BDF::DevMin; Dev != BDF::DevMax; ++Dev) {
      for (uint16_t Fun = BDF::FunMin; Fun != BDF::FunMax; ++Fun) {
        BDF BDF(Bus, Dev, Fun);
        bool Return = Fn(BDF);
        if (Return)
          return;
      }
    }
  }
}

void PCI::listDevices(std::ostream &OS) {
  forEachBDF([&OS](const BDF &BDF) -> bool {
    // Reading a single double-word should be faster than reading two words, one
    // for Vendor ID and one for Device ID.
    uint32_t DWord = readDword(BDF, VendorIdReg);
    uint16_t VendorId = DWord & 0x0000ffff;
    uint16_t DeviceId = DWord & 0xffff0000;
    OS << BDF << std::hex << VendorId << ":" << DeviceId << std::dec << "\n";
    // Don't stop iterating.
    return false;
  });
}

void PCI::listDevices() { listDevices(std::cout); }

