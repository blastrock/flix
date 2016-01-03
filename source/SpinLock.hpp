#ifndef SPIN_LOCK_HPP
#define SPIN_LOCK_HPP

#include "Util.hpp"
#include "Debug.hpp"

// no need to spin, SMP not supported yet
class SpinLock
{
public:
  using Scoped = ScopedLock<SpinLock>;
  using ScopedUnlock = ScopedUnlock<SpinLock>;

  SpinLock() = default;
  SpinLock(const SpinLock&) = delete;
  SpinLock(SpinLock&&) = delete;
  SpinLock& operator=(const SpinLock&) = delete;
  SpinLock& operator=(SpinLock&&) = delete;

  Scoped getScoped()
  {
    return Scoped(*this);
  }

  ScopedUnlock getScopedUnlock()
  {
    return ScopedUnlock(*this);
  }

  void lock()
  {
    xDebC("support/sync/spinlock", "%p: SpinLock lock", this);
    assert(!_disableInterrupts);
    _disableInterrupts = DisableInterrupts();
#ifndef NDEBUG
    ++g_lockCount;
#endif
  }

  void unlock()
  {
    xDebC("support/sync/spinlock", "%p: SpinLock unlock", this);
    assert(_disableInterrupts);
    // do not reenable interrupts before we reset the optional or the assert in
    // lock() may fail
#ifndef NDEBUG
    --g_lockCount;
#endif
    auto di = std::move(*_disableInterrupts);
    _disableInterrupts = std::nullopt;
  }

#ifndef NDEBUG
  static uint8_t getLockCount()
  {
    return g_lockCount;
  }
#endif

private:
  std::optional<DisableInterrupts> _disableInterrupts;

#ifndef NDEBUG
  static uint8_t g_lockCount;
#endif
};

#endif
