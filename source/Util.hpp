#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include "Cpu.hpp"

#include <experimental/optional>

namespace std
{
  using experimental::optional;
  using experimental::nullopt;
}

template <typename T>
inline T* ptrAdd(T* ptr, intptr_t val)
{
  return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr)+val);
}

template <typename T1, typename T2>
inline uintptr_t ptrDiff(T1* ptr1, T2* ptr2)
{
  return reinterpret_cast<char*>(ptr1)-reinterpret_cast<char*>(ptr2);
}

/**
 * Align a pointer to val bytes above
 *
 * \example If called with 0x9 and 8, it will return 0x10.
 *
 * \warning val must be a power of two
 */
template <typename T>
inline T* ptrAlignSup(T* ptr, uint8_t val)
{
  return reinterpret_cast<T*>((reinterpret_cast<uintptr_t>(ptr) + val-1)
      & ~(uintptr_t)(val-1));
}

/**
 * Align an integer to val bytes above
 *
 * \example If called with 0x9 and 8, it will return 0x10.
 *
 * \warning val must be a power of two
 */
template <typename T>
inline constexpr T intAlignSup(T base, uint8_t val)
{
  return (base + val-1) & ~(T)(val-1);
}

// must use a memory barrier, disabling interrupts is like a lock, memory
// accesses must not be scheduled before or after the critical section
inline void disableInterrupts()
{
  asm volatile ("cli":::"memory");
}

inline void enableInterrupts()
{
  asm volatile ("sti":::"memory");
}

class DisableInterrupts
{
public:
  DisableInterrupts()
  {
    _enable = Cpu::rflags() & (1 << 9);
    if (_enable)
      disableInterrupts();
  }
  ~DisableInterrupts()
  {
    if (_enable)
      enableInterrupts();
  }

  DisableInterrupts(const DisableInterrupts&) = delete;
  DisableInterrupts(DisableInterrupts&&) = default;
  DisableInterrupts& operator=(const DisableInterrupts&) = delete;
  DisableInterrupts& operator=(DisableInterrupts&&) = default;

private:
  bool _enable;
};

class EnableInterrupts
{
public:
  EnableInterrupts()
  {
    _disable = !(Cpu::rflags() & (1 << 9));
    if (_disable)
      enableInterrupts();
  }
  ~EnableInterrupts()
  {
    if (_disable)
      disableInterrupts();
  }

  EnableInterrupts(const EnableInterrupts&) = delete;
  EnableInterrupts(EnableInterrupts&&) = default;
  EnableInterrupts& operator=(const EnableInterrupts&) = delete;
  EnableInterrupts& operator=(EnableInterrupts&&) = default;

private:
  bool _disable;
};

template <typename T>
class ScopedLock
{
public:
  ScopedLock(T& lock)
    : lock(lock)
  {
    lock.lock();
  }
  ~ScopedLock()
  {
    lock.unlock();
  }

  ScopedLock(const ScopedLock&) = delete;
  ScopedLock(ScopedLock&&) = default;
  ScopedLock& operator=(const ScopedLock&) = delete;
  ScopedLock& operator=(ScopedLock&&) = default;

private:
  T& lock;
};

template <typename T>
class ScopedUnlock
{
public:
  ScopedUnlock(T& lock)
    : lock(lock)
  {
    lock.unlock();
  }
  ~ScopedUnlock()
  {
    lock.lock();
  }

  ScopedUnlock(const ScopedUnlock&) = delete;
  ScopedUnlock(ScopedUnlock&&) = default;
  ScopedUnlock& operator=(const ScopedUnlock&) = delete;
  ScopedUnlock& operator=(ScopedUnlock&&) = default;

private:
  T& lock;
};

#endif /* UTIL_HPP */
