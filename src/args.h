//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __SRC_ARGS_H__
#define __SRC_ARGS_H__

#include "freqentry.h"
#include <iomanip>
#include <iostream>
#include <string>

struct Arguments {
  /// The FSB/SDRAM/PCI frequencies.
  FreqEntry Fsb;
  /// The PLL Name.
  std::string PLL;
  void print(std::ostream &OS) const;
  friend std::ostream &operator<<(std::ostream &OS, const Arguments &Args) {
    Args.print(OS);
    return OS;
  }
};

#endif // __SRC_ARGS_H__
