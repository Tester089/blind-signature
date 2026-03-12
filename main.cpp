#include <iostream>
#include "core.h"

int main() {
  BigInt a(100), b(200);
  BigInt c = add(a, b);
  std::cout << "100 + 200 = " << to_hex(c) << "\n";
  return 0;
}