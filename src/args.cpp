//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include "args.h"

void Arguments::print(std::ostream &OS) const {
  OS << "FSB/SDRAM/PCI: " << Fsb << std::endl;
  OS << "PLL: " << PLL << std::endl;
}
