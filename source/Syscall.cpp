#include "Syscall.hpp"
#include "PageDirectory.hpp"
#include "Task.hpp"
#include <array>
#include <functional>

namespace sys
{

static std::array<std::function<void(const InterruptState& state)>, last_id>
  g_syscallHandlers;

namespace detail
{

void registerHandler(ScId scid,
    std::function<void(const InterruptState&)> handler)
{
  auto id = static_cast<unsigned>(scid);
  assert(id < last_id);
  g_syscallHandlers[id] = std::move(handler);
}

}

namespace hndl
{

void exit()
{
  // terminating a task will free its page directory, so we need to switch
  // to the kernel one before we do that
  PageDirectory::getKernelDirectory()->use();
  TaskManager::get()->terminateCurrentTask();
  TaskManager::get()->scheduleNext();
}

void print(const char* buf)
{
  Degf("sysprint: %s", buf);
}

}

void initSysCalls()
{
  registerHandler(exit, hndl::exit);
  registerHandler(print, hndl::print);
}

void handle(const InterruptState& st)
{
  assert(st.rax < last_id);
  assert(g_syscallHandlers[st.rax]);
  g_syscallHandlers[st.rax](st);
}

}
