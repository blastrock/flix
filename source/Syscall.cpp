#include "Syscall.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "DescTables.hpp"
#include <array>
#include <functional>

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
  auto& task = TaskManager::get()->getCurrentTask();
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
  auto& task = TaskManager::get()->getCurrentTask();
  return task.fileManager.close(fd);
}

ssize_t read(int fd, void* buf, size_t count)
{
  Degf("reading fd %d", fd);

  auto& task = TaskManager::get()->getCurrentTask();
  auto handle = task.fileManager.getHandle(fd);

  if (!handle)
  {
    Degf("fd not found");
    return -1;
  }

  return handle->read(buf, count);
}

ssize_t write(int fd, const void* buf, size_t count)
{
  Degf("writing \"%s\" on fd %d",
      std::string(static_cast<const char*>(buf), count).c_str(),
      fd);

  auto& task = TaskManager::get()->getCurrentTask();
  auto handle = task.fileManager.getHandle(fd);

  if (!handle)
  {
    Degf("fd not found");
    return -1;
  }

  return handle->write(buf, count);
}

int arch_prctl(int code, unsigned long addr)
{
  static constexpr uint32_t MSR_FS = 0xC0000100;
  if (code == 0x1002)
  {
    Degf("arch_prctl: set fs to %x", addr);
    asm volatile (
        "wrmsr\n"
        :
        :"c"(MSR_FS)
        ,"d"(static_cast<uint32_t>(addr >> 32))
        ,"a"(static_cast<uint32_t>(addr))
        );
  }
  else
    Degf("arch_prctl: unknown code 0x%x", code);
  return 0;
}

void* mmap(void*, size_t length)
{
  Degf("mmap: size %x", length);

  if (length == 0)
    return nullptr;

  static uintptr_t curPtr = 0xffffffff00005000;

  void* start = reinterpret_cast<void*>(curPtr);

  size_t nbPages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
  auto& pd = TaskManager::get()->getCurrentTask().pageDirectory;
  while (nbPages--)
  {
    pd.mapPage(reinterpret_cast<void*>(curPtr),
        PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);
    curPtr += PAGE_SIZE;
  }

  Degf("returning %p", start);
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
  Degf("sysprint: %s", buf);
}

}

static constexpr uint32_t MSR_EFER = 0xC0000080;
static constexpr uint32_t MSR_STAR = 0xC0000081;
static constexpr uint32_t MSR_LSTAR = 0xC0000082;

static void initSysCallGate()
{
  // sysret sets the cs to SYSTEM_CS + 16, don't know why...
  uint64_t star =
    (static_cast<uint64_t>(DescTables::USER_CS - 16) << 48) |
    (static_cast<uint64_t>(DescTables::SYSTEM_CS) << 32);

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
  assert(st.rax < last_id);
  if (g_syscallHandlers[st.rax])
    return g_syscallHandlers[st.rax](st);
  else
  {
    Degf("Unknown syscall %d", st.rax);
    return 0;
  }
}

}

extern "C" void syscallHandler(InterruptState* s)
{
  Degf("syscall %d", s->rax);
  s->rax = sys::handle(*s);
}
