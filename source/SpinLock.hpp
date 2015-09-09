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
    xDebC("support/sync/spinlock", "SpinLock lock");
    assert(!_disableInterrupts);
    _disableInterrupts = DisableInterrupts();
  }

  void unlock()
  {
    xDebC("support/sync/spinlock", "SpinLock unlock");
    assert(_disableInterrupts);
    _disableInterrupts = std::nullopt;
  }

private:
  std::optional<DisableInterrupts> _disableInterrupts;
};

#endif
