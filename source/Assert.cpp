#include <cassert>
#include "Debug.hpp"

namespace std_impl
{
  void assertFail(const char* file, int line, const char* condition)
  {
    fDeg() << file << ':' << line << "\nAssertion failed: " << condition;
  }
}
