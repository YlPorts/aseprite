#include "base/memory.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

int main() {
  for (std::size_t alignment : {std::size_t(16), std::size_t(64), std::size_t(4096)}) {
    for (std::size_t bytes : {std::size_t(1), std::size_t(37), std::size_t(10000)}) {
      void* mem = base_aligned_alloc(bytes, alignment);
      assert(mem && reinterpret_cast<std::uintptr_t>(mem) % alignment == 0);
      std::memset(mem, 0xa5, bytes);
      base_aligned_free(mem);
    }
  }
  assert(base_aligned_alloc(64, 3) == nullptr);
  base_aligned_free(nullptr);
  std::cout << "PASS: original LAF Android aligned allocation, writes and free\n";
}
