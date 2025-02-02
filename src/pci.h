//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//
// For a great overview of how we access PCI devices please refer to:
// https://en.wikipedia.org/wiki/PCI_configuration_space
//

#ifndef __SRC_PCI_H__
#define __SRC_PCI_H__

#include <cstdint>
#include <functional>
#include <iostream>
#include "utils.h"

/// Geographical addressing of PCI devices.
struct BDF {
  /// PCI Bus.
  uint16_t Bus;
  /// PCI Device.
  uint16_t Dev;
  /// PCI Function.
  uint16_t Fun;

  static constexpr const uint16_t BusMin = 0;
  static constexpr const uint16_t BusMax = 256;
  static constexpr const uint16_t DevMin = 0;
  static constexpr const uint16_t DevMax = 32;
  static constexpr const uint16_t FunMin = 0;
  static constexpr const uint16_t FunMax = 8;

  BDF(uint16_t Bus, uint16_t Dev, uint16_t Fun)
      : Bus(Bus), Dev(Dev), Fun(Fun) {}
  /// \Returns the address that corresponds to this BDF.
  uint32_t getAddr(uint16_t Reg = 0) const {
    return 0x80000000 | Bus << 16 | Dev << 11 | Fun << 8 | (Reg & 0xfc);
  }
  friend std::ostream &operator<<(std::ostream &OS, const BDF &BDF) {
    OS << BDF.Bus << ":" << BDF.Dev << ":" << BDF.Fun;
    return OS;
  }
  bool operator==(const BDF &Other) const {
    return Bus == Other.Bus && Dev == Other.Dev && Fun == Other.Fun;
  }
  bool operator!=(const BDF &Other) const { return !(*this == Other); }
};

class PCI {
public:
  /// The I/O registers used for legacy reads/writes to PCI.
  /// The address is written to PCI_CONFIG_ADDR registers and the data to
  /// PCI_CONFIG_DATA register.
  static constexpr const uint16_t PCI_CONFIG_ADDR = 0xcf8;
  static constexpr const uint16_t PCI_CONFIG_DATA = 0xcfc;
  /// The word containging the Vendor ID.
  static constexpr const uint16_t VendorIdReg = 0;
  /// The word containging the Device ID.
  static constexpr const uint16_t DeviceIdReg = 2;

  static uint8_t readByte(const BDF &BDF, uint16_t Reg) {
    uint32_t Addr = BDF.getAddr(Reg);
    outportl(PCI_CONFIG_ADDR, Addr);
    return inportb(PCI_CONFIG_DATA + (Reg & 0x03));
  }
  static uint16_t readWord(const BDF &BDF, uint16_t Reg) {
    uint16_t Res = readByte(BDF, Reg);
    Res |= readByte(BDF, Reg + 1) << 8;
    return Res;
  }
  static uint32_t readDword(const BDF &BDF, uint16_t Reg) {
    uint32_t Addr = BDF.getAddr(Reg);
    outportl(PCI_CONFIG_ADDR, Addr);
    return inportl(PCI_CONFIG_DATA + (Reg & 0x03));
  }
  static void writeByte(const BDF &BDF, uint16_t Reg, uint8_t Val) {
    uint32_t Addr = BDF.getAddr(Reg);
    outportl(PCI_CONFIG_ADDR, Addr);
    outportb(PCI_CONFIG_DATA + (Reg & 0x03), Val);
  }
  static void writeWord(const BDF &BDF, uint16_t Reg, uint16_t Val) {
    writeByte(BDF, Reg, Val);
    writeByte(BDF, Reg + 1, Val >> 8);
  }
  static void writeDword(const BDF &BDF, uint16_t Reg, uint32_t Val) {
    uint32_t Addr = BDF.getAddr(Reg);
    outportl(PCI_CONFIG_ADDR, Addr);
    outportl(PCI_CONFIG_DATA + (Reg & 0x03), Val);
  }
  /// Runs \p Fn on each available BDF in the PCI address range. If \p Fn()
  /// returns true the iteration stops.
  static void forEachBDF(std::function<bool(const BDF &)> Fn);
  /// Prints all PCI devices.
  static void listDevices(std::ostream &OS);
  /// Prints all PCI devices to std::cout.
  static void listDevices();
};


#endif // __SRC_PCI_H__
