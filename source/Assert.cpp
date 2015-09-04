#include <cassert>
#include "Debug.hpp"

XLL_LOG_CATEGORY("support/assert");

extern "C" void assert_fail(const char* file, int line, const char* condition)
{
  xDeb("%s:%d\nAssertaion failed: %s", file, line, condition);
  printStackTrace();
  PANIC("Assertion failed");
}
