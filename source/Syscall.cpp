#include "Syscall.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "DescTables.hpp"
#include <array>
#include <functional>

extern "C" void syscall_entry();

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

static constexpr uint32_t MSR_EFER = 0xC0000080;
static constexpr uint32_t MSR_STAR = 0xC0000081;
static constexpr uint32_t MSR_LSTAR = 0xC0000082;

static void initSysCallGate()
{
  uint64_t star =
    static_cast<uint64_t>(DescTables::USER_CS) << 48 |
    static_cast<uint64_t>(DescTables::SYSTEM_CS) << 32;

  asm volatile ("wrmsr"
      :
      :"c"(MSR_STAR)
      ,"d"(static_cast<uint32_t>(star >> 32))
      ,"a"(static_cast<uint32_t>(star))
     );

  uint64_t lstar = reinterpret_cast<uint64_t>(&syscall_entry);

  asm volatile ("wrmsr"
      :
      :"c"(MSR_LSTAR)
      ,"d"(static_cast<uint32_t>(lstar >> 32))
      ,"a"(static_cast<uint32_t>(lstar))
     );

  asm volatile (
      "rdmsr\n"
      "btsl $0, %%eax\n"
      "wrmsr\n"
      :
      :"c"(MSR_EFER)
      :"ecx", "eax", "flags"
      );
}

void initSysCalls()
{
  initSysCallGate();

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

extern "C" void syscallHandler()
{
  Degf("SYSCALL GATE");
}
