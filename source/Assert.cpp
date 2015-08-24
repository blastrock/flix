#include <cassert>
#include "Debug.hpp"

extern "C" void assert_fail(const char* file, int line, const char* condition)
{
  Degf("%s:%d\nAssertaion failed: %s", file, line, condition);
  printStackTrace();
  PANIC("Assertion failed");
}
