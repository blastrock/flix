#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include "Cpu.hpp"
#include "Debug.hpp"

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
inline T* ptrAlignSup(T* ptr, uint32_t val)
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
inline constexpr T intAlignSup(T base, uint32_t val)
{
  return (base + val-1) & ~(T)(val-1);
}

// must use a memory barrier, disabling interrupts is like a lock, memory
// accesses must not be scheduled before or after the critical section
inline void disableInterrupts()
{
  xDebC("support/utils/interrupts", "Disabling interrupts");
  assert((Cpu::rflags() & (1 << 9)) && "interrupts were already off");
  asm volatile ("cli":::"memory");
}

inline void enableInterrupts()
{
  xDebC("support/utils/interrupts", "Enabling interrupts");
  assert(!(Cpu::rflags() & (1 << 9)) && "interrupts were already on");
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
  DisableInterrupts(DisableInterrupts&& other)
    : _enable(other._enable)
  {
    other._enable = false;
  }
  DisableInterrupts& operator=(const DisableInterrupts&) = delete;
  DisableInterrupts& operator=(DisableInterrupts&& other)
  {
    _enable = other._enable;
    other._enable = false;
    return *this;
  }

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
  EnableInterrupts(EnableInterrupts&& other)
    : _disable(other._disable)
  {
    other._disable = false;
  }
  EnableInterrupts& operator=(const EnableInterrupts&) = delete;
  EnableInterrupts& operator=(EnableInterrupts&& other)
  {
    _disable = other._disable;
    other._disable = false;
    return *this;
  }

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
    if (!_dead)
      lock.unlock();
  }

  ScopedLock(const ScopedLock&) = delete;
  ScopedLock(ScopedLock&& other)
    : lock(other.lock)
  {
    other._dead = true;
  }
  ScopedLock& operator=(const ScopedLock&) = delete;
  ScopedLock& operator=(ScopedLock&& other)
  {
    other._dead = true;
    return *this;
  }

private:
  bool _dead = false;
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
    if (!_dead)
      lock.lock();
  }

  ScopedUnlock(const ScopedUnlock&) = delete;
  ScopedUnlock(ScopedUnlock&& other)
    : lock(other.lock)
  {
    other._dead = true;
  }
  ScopedUnlock& operator=(const ScopedUnlock&) = delete;
  ScopedUnlock& operator=(ScopedUnlock&& other)
  {
    other._dead = true;
    return *this;
  }

private:
  bool _dead = false;
  T& lock;
};

#endif /* UTIL_HPP */
