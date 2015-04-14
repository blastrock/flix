#include "Debug.hpp"

void printStackTrace(uint64_t stackPointer)
{
  void** stackFrame = reinterpret_cast<void**>(stackPointer);

  Degf("Stack trace:");
  while (stackFrame[1])
  {
    Degf("%p", stackFrame[1]);
    stackFrame = static_cast<void**>(stackFrame[0]);
  }
}
