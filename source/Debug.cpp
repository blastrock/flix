#include "Debug.hpp"

void printStackTrace(uint64_t stackPointer)
{
  void** stackFrame = reinterpret_cast<void**>(stackPointer);
  void* returnAddress;

  fDeg() << "Stack trace:";
  while (stackFrame[1])
  {
    fDeg() << stackFrame[1];
    stackFrame = static_cast<void**>(stackFrame[0]);
  }
}
