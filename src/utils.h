//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __SRC_UTILS_H__
#define __SRC_UTILS_H__

#include <ostream>
#include <string>

#ifdef LINUX
// Linux
#include <unistd.h>
#include <cstdint>
static inline void outportl(uint16_t Reg, uint32_t Val) { }
static inline void outportb(uint16_t Reg, uint8_t Val) { }
static inline uint8_t inportb(uint16_t Reg) { return 0; }
static inline uint32_t inportl(uint16_t Reg) { return 0; }

#else

// DOS
#include <dos.h>
#include <inlines/pc.h>

#endif // LINUX

/// Sleep for \p Millis milliseconds with a busy loop.
void delay(unsigned Millis);

/// Converts \p Str to lower case and returns it.
std::string toLower(const std::string &Str);

/// Whether we should print debug info.
extern bool Debug;

class DecimalGuard {
  std::ostream &OS;
  std::ostream::fmtflags SvFlags;
public:
  DecimalGuard(std::ostream &OS) : OS(OS) {
    SvFlags = OS.flags();
    OS << std::dec;
  }
  ~DecimalGuard() { OS.flags(SvFlags); }
};

#endif // __SRC_UTILS_H__
