//-*- C++ -*-
//
// Copyright (C) 2025 Scrap Computing
//

#include <iostream>
#include "sisfsb.h"
#include "args.h"

static constexpr const char *VERSION = "0.1";

static void usage() {
  static const char *BinName = "sisfsb";
  std::cerr << "Usage:" << std::endl;
  std::cerr << BinName << " -pll <PLL | help> -fsb <FSB/SDRAM/PCI|"
            << FreqEntry::ListStr << "> [-h|-help] [-debug] [-v|-version]"
            << std::endl;
}

static bool parseOpts(int Argc, char **Argv, Arguments &Args) {
  for (int ArgIdx = 1; ArgIdx < Argc; ++ArgIdx) {
    std::string Arg(toLower(Argv[ArgIdx]));
    auto MatchArg = [](const std::string &ArgCandidate, const char *ArgRaw,
                       const char *AltArgRaw = nullptr) -> bool {
      std::string Arg(ArgRaw);
      if (ArgCandidate == "-" + Arg || ArgCandidate == "--" + Arg)
        return true;
      if (AltArgRaw != nullptr) {
        std::string AltArg(ArgRaw);
        if (ArgCandidate == "-" + AltArg || ArgCandidate == "--" + AltArg)
          return true;
      }
      return false;
    };
    // Returns the next argument or nullopt if not found.
    auto TryGetNextArg = [ArgIdx, Argv, Argc]() -> std::optional<std::string> {
      if (ArgIdx + 1 >= Argc)
        return std::nullopt;
      return Argv[ArgIdx + 1];
    };
    if (MatchArg(Arg, "help", "h"))
      return false;
    if (MatchArg(Arg, "version", "v")) {
      std::cout << "Version: " << VERSION << std::endl;
      exit(0);
    }
    if (MatchArg(Arg, "pll")) {
      if (auto ArgStrOpt = TryGetNextArg())
        Args.PLL = *ArgStrOpt;
      else {
        std::cerr << "Missing PLL name!" << std::endl;
        return false;
      }
      continue;
    }
    if (MatchArg(Arg, "fsb")) {
      if (auto ArgStrOpt = TryGetNextArg())
        Args.Fsb = FreqEntry(Argv[ArgIdx + 1]);
      else {
        std::cerr << "Missing FSB argument!" << std::endl;
        return false;
      }
      if (Args.Fsb.bad() && toLower(Args.Fsb.getFreqStr()) != FreqEntry::ListStr)
        return false;
      continue;
    }
    if (MatchArg(Arg, "debug")) {
      Debug = true;
      continue;
    }
  }
  return true;
}

bool Debug = false;

int main(int Argc, char **Argv) {
  std::cout << R"( ____   _  ____   _____  ____   ____  )" << std::endl;
  std::cout << R"(/ ___| (_)/ ___| |  ___|/ ___| | __ ) )" << std::endl;
  std::cout << R"(\___ \ | |\___ \ | |_   \___ \ |  _ \ )" << std::endl;
  std::cout << R"( ___) || | ___) ||  _|   ___) || |_) |)" << std::endl;
  std::cout << R"(|____/ |_||____/ |_|    |____/ |____/ )" << std::endl;
  std::cout << R"(                     by Scrap Computing  v)" << VERSION << std::endl;
  Arguments Args;
  if (!parseOpts(Argc, Argv, Args)) {
    usage();
    return 1;
  }
  std::cout << Args << std::endl;
  SiSFSB SiSFSB(Args);
  bool Success = SiSFSB.run();
  return Success ? 0 : 1;
}
