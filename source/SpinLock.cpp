#include "SpinLock.hpp"

#ifndef NDEBUG
uint8_t SpinLock::g_lockCount = 0;
#endif
