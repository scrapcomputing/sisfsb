//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#ifndef __SRC_FREQENTRY_H__
#define __SRC_FREQENTRY_H__

#include <ostream>
#include <string>

class FreqEntry {
  bool Bad = true;
  std::string FreqStr;
  float Fsb = 0.0;
  float Sdram = 0.0;
  float Pci = 0.0;
  enum class ParseState {
    FSB,
    SDRAM,
    PCI,
    DONE,
  };
  static constexpr const char Delim = '/';
  /// Helper equality with acceptable error.
  static bool Eq(float V1, float V2, float Err) {
    auto Abs = [](float V) { return V >= 0 ? V : -V; };
    return Abs(V1 - V2) <= Abs(Err);
  }

public:
  static constexpr const char *ListStr = "list";
  FreqEntry() = default;
  FreqEntry(const std::string &OrigStr);
  FreqEntry(float Fsb, float Sdram, float Pci)
    : Bad(false), Fsb(Fsb), Sdram(Sdram), Pci(Pci) {
    std::string DS(1, Delim);
    FreqStr = std::to_string(Fsb) + DS + std::to_string(Sdram) + DS +
              std::to_string(Pci);
  }
  bool bad() const { return Bad; }
  const std::string getFreqStr() const { return FreqStr; }
  float getFsb() const { return Fsb; }
  float getSdram() const { return Sdram; }
  float getPci() const { return Pci; }
  void print(std::ostream &OS) const;
  friend std::ostream &operator<<(std::ostream &OS, const FreqEntry &F) {
    F.print(OS);
    return OS;
  }
  bool operator==(const FreqEntry &Other) const {
    static constexpr const float Err = 2;
    return Bad == Other.Bad && Eq(Fsb, Other.Fsb, Err) &&
           Eq(Sdram, Other.Sdram, Err) && Eq(Pci, Other.Pci, Err);
  }
};

#endif // __SRC_FREQENTRY_H__
