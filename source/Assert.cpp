#include <cassert>
#include "Debug.hpp"

namespace std_impl
{
  void assertFail(const char* file, int line, const char* condition)
  {
    Degf("%s:%d\nAssertaion failed: %s", file, line, condition);
  }
}
