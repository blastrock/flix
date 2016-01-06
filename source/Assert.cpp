#include <cassert>
#include "Util.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("support/assert");

extern "C"[[noreturn]] void assert_fail(
    const char* file, int line, const char* condition)
{
  asm volatile ("cli");
  xDeb("%s:%d\nAssertion failed: %s", file, line, condition);
  printStackTrace();
  PANIC("Assertion failed");
}
