
#include "utils.h"
#include <algorithm>
#include <chrono>

void delay(unsigned Millis) {
  auto Start = std::chrono::high_resolution_clock::now();
  while (true) {
    auto Now = std::chrono::high_resolution_clock::now();
    double Elapsed = std::chrono::duration<double, std::milli>(Now - Start).count();
    if (Elapsed > Millis)
      break;
  }
}

std::string toLower(const std::string &Str) {
  std::string NewStr(Str);
  std::transform(NewStr.begin(), NewStr.end(), NewStr.begin(),
                 [](unsigned char C) { return std::tolower(C); });
  return NewStr;
}
