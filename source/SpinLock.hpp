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
  }

  void unlock()
  {
    xDebC("support/sync/spinlock", "%p: SpinLock unlock", this);
    assert(_disableInterrupts);
    // do not reenable interrupts before we reset the optional or the assert in
    // lock() may fail
    auto di = std::move(*_disableInterrupts);
    _disableInterrupts = std::nullopt;
  }

private:
  std::optional<DisableInterrupts> _disableInterrupts;
};

#endif
