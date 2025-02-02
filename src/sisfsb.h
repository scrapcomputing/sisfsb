//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __SRC_SISFSB_H__
#define __SRC_SISFSB_H__

#include "args.h"
#include "chips.h"

class PLL;

class SiSFSB {
  Arguments &Args;
  Chips AllChips;

  PLL *findPLL() const;

public:
  SiSFSB(Arguments &Args) : Args(Args) {}
  /// \Returns true on success, false if an error occured.
  bool run();
};

#endif // __SRC_SISFSB_H__
