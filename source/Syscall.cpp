#include "Syscall.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "DescTables.hpp"
#include "Cpu.hpp"
#include <array>
#include <functional>

XLL_LOG_CATEGORY("core/syscall");

extern "C" void syscall_entry();

namespace sys
{

static std::array<SyscallHandler, last_id> g_syscallHandlers;

namespace detail
{

void registerHandler(ScId scid,
    std::function<SyscallReturnType(const InterruptState&)> handler)
{
  auto id = static_cast<unsigned>(scid);
  assert(id < last_id);
  g_syscallHandlers[id] = std::move(handler);
}

}

namespace hndl
{

int open(const char* path)
{
  auto& task = TaskManager::get()->getActiveTask();
  auto expfd = task.fileManager.open(path);
  if (!expfd)
    return -1;
  return *expfd;
}

int openat(int dirfd, const char* path)
{
  (void)dirfd;
  return open(path);
}

int close(int fd)
{
  auto& task = TaskManager::get()->getActiveTask();
  return task.fileManager.close(fd);
}

ssize_t read(int fd, void* buf, size_t count)
{
  xDeb("reading fd %d", fd);

  auto& task = TaskManager::get()->getActiveTask();
  auto handle = task.fileManager.getHandle(fd);

  if (!handle)
  {
    xDeb("fd not found");
    return -1;
  }

  auto readResult = handle->read(buf, count);
  if (!readResult)
    return -1;
  return *readResult;
}

ssize_t write(int fd, const void* buf, size_t count)
{
  xDeb("writing \"%s\" on fd %d",
      std::string(static_cast<const char*>(buf), count).c_str(),
      fd);

  auto& task = TaskManager::get()->getActiveTask();
  auto handle = task.fileManager.getHandle(fd);

  if (!handle)
  {
    xDeb("fd not found");
    return -1;
  }

  auto writeResult = handle->write(buf, count);
  if (!writeResult)
    return -1;
  return *writeResult;
}

int arch_prctl(int code, unsigned long addr)
{
  static constexpr uint32_t MSR_FS = 0xC0000100;
  if (code == 0x1002)
  {
    xDeb("arch_prctl: set fs to %x", addr);
    asm volatile (
        "wrmsr\n"
        :
        :"c"(MSR_FS)
        ,"d"(static_cast<uint32_t>(addr >> 32))
        ,"a"(static_cast<uint32_t>(addr))
        );
  }
  else
    xDeb("arch_prctl: unknown code 0x%x", code);
  return 0;
}

void* mmap(void*, size_t length)
{
  xDeb("mmap: size %x", length);

  if (length == 0)
    return nullptr;

  static uintptr_t curPtr = 0xffffffff00005000;

  void* start = reinterpret_cast<void*>(curPtr);

  size_t nbPages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
  auto& pd = TaskManager::get()->getActiveTask().pageDirectory;
  while (nbPages--)
  {
    pd.mapPage(reinterpret_cast<void*>(curPtr),
        PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC |
        PageDirectory::ATTR_DEFER);
    curPtr += PAGE_SIZE;
  }

  xDeb("returning %p", start);
  return start;
}

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
  xDeb("sysprint: %s", buf);
}

}

static void initSysCallGate()
{
  // sysret sets the cs to SYSTEM_CS + 16, don't know why...
  uint64_t star =
    (static_cast<uint64_t>(DescTables::USER_CS - 16) << 48) |
    (static_cast<uint64_t>(DescTables::SYSTEM_CS) << 32);

  Cpu::writeMsr(Cpu::MSR_STAR, star);

  uint64_t lstar = reinterpret_cast<uint64_t>(&syscall_entry);

  Cpu::writeMsr(Cpu::MSR_LSTAR, lstar);

  uint64_t efer = Cpu::readMsr(Cpu::MSR_EFER);
  efer |= 0x1; // enable syscall/sysret
  Cpu::writeMsr(Cpu::MSR_EFER, efer);
}

void initSysCalls()
{
  initSysCallGate();

  registerHandler(read, hndl::read);
  registerHandler(write, hndl::write);
  registerHandler(open, hndl::open);
  registerHandler(close, hndl::close);
  registerHandler(mmap, hndl::mmap);
  registerHandler(arch_prctl, hndl::arch_prctl);
  registerHandler(exit, hndl::exit);
  registerHandler(openat, hndl::openat);
  registerHandler(print, hndl::print);
}

SyscallReturnType handle(const InterruptState& st)
{
  assert((st.cs == DescTables::SYSTEM_CS ||
        (st.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");

  assert(st.rax < last_id);
  if (g_syscallHandlers[st.rax])
    return g_syscallHandlers[st.rax](st);
  else
  {
    xDeb("Unknown syscall %d", st.rax);
    return 0;
  }
}

}

extern "C" void syscallHandler(InterruptState* s)
{
  xDeb("syscall %d", s->rax);
  s->rax = sys::handle(*s);
}
