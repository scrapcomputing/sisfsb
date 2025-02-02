//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "freqentry.h"
#include "utils.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

FreqEntry::FreqEntry(const std::string &OrigStr) : FreqStr(OrigStr) {
  std::string Str = OrigStr + Delim;
  ParseState State = ParseState::FSB;
  unsigned Start = 0;
  if (OrigStr == FreqEntry::ListStr) {
    Bad = true;
  } else {
    for (unsigned Idx = 0, E = Str.size(); Idx != E; ++Idx) {
      char C = Str[Idx];
      if (C == Delim) {
        float *Dst = nullptr;
        switch (State) {
        case ParseState::FSB:
          Dst = &Fsb;
          State = ParseState::SDRAM;
          break;
        case ParseState::SDRAM:
          Dst = &Sdram;
          State = ParseState::PCI;
          break;
        case ParseState::PCI:
          Dst = &Pci;
          State = ParseState::DONE;
          break;
        case ParseState::DONE:
          std::cerr << "Error parsing frequencies: <FSB>" << Delim << "<SDRAM>"
                    << Delim << "<PCI>, too many elements!" << std::endl;
          Bad = true;
          return;
        }
        std::string ArgStr = Str.substr(Start, Idx - Start);
        double Float = std::atof(ArgStr.c_str());
        if (Float <= 0.0 || Float >= 1000) {
          std::cerr << "Invalid frequency '" << ArgStr << "' !" << std::endl;
          Bad = true;
          return;
        }
        *Dst = Float;
        Start = Idx + 1;
      }
    }
    if (State != ParseState::DONE) {
      std::cerr << "Error parsing frequencies: <FSB>" << Delim << "<SDRAM>"
                << Delim << "<PCI>, too few elements!" << std::endl;
      Bad = true;
      return;
    }
    Bad = false;
  }
}

void FreqEntry::print(std::ostream &OS) const {
  DecimalGuard DG(OS);
  OS << std::fixed << std::setprecision(1);
  if (Bad)
    OS << "BAD";
  else
    OS << std::setw(5) << Fsb << Delim << std::setw(5) << Sdram << Delim
       << std::setw(4) << Pci << " (Div:" << Fsb / Pci << ")";
}
