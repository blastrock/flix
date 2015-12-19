#include "Syscall.hpp"
#include "PageDirectory.hpp"
#include "TaskManager.hpp"
#include "DescTables.hpp"
#include "Cpu.hpp"
#include "Elf.hpp"
#include <array>
#include <functional>
#include <flix/stat.h>

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

  static uintptr_t curPtr = 0x00000000ff005000;

  void* start = reinterpret_cast<void*>(curPtr);

  size_t nbPages = (length + PAGE_SIZE - 1) / PAGE_SIZE;
  auto& pd = TaskManager::get()->getActiveTask().pageDirectory;
  while (nbPages--)
  {
    pd.mapPage(reinterpret_cast<void*>(curPtr),
        PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC |
        PageDirectory::ATTR_DEFER | PageDirectory::ATTR_NOEXEC);
    curPtr += PAGE_SIZE;
  }

  xDeb("returning %p", start);
  return start;
}

pid_t wait4(pid_t pid, int* status, int options, struct rusage* rusage)
{
  return TaskManager::get()->wait(pid, status);
}

void exit()
{
  TaskManager::get()->terminateCurrentTask();
}

void print(const char* buf)
{
  xDeb("sysprint: %s", buf);
}

int fstat()
{
  xErr("fstat not implemented");
  return -1;
}

int newfstatat(int dirfd, const char* pathname, struct stat* buf, int flags)
{
  xDeb("newfstatat(%d, \"%s\", %p, %d)", dirfd, pathname, buf, flags);

  const fs::LookupOptions options = (flags & AT_SYMLINK_NOFOLLOW)
                                        ? fs::LookupOptions_NoFollowSymlink
                                        : fs::LookupOptions_None;

  fs::IoExpected<std::shared_ptr<fs::Inode>> exptarget;
  if (pathname[0] == '/')
  {
    exptarget = fs::lookup(nullptr, pathname, options);
  }
  else if (dirfd >= 0)
  {
    if (*pathname == '\0')
    {
      xErr("empty pathname not supported in newfstatat");
      return -1;
    }

    auto& task = TaskManager::get()->getActiveTask();
    auto handle = task.fileManager.getHandle(dirfd);
    if (!handle)
    {
      xDeb("fd not found");
      return -1;
    }

    auto expinode = handle->getInode();
    if (!expinode)
    {
      xDeb("No inode associated");
      return -1;
    }

    auto inode = *expinode;
    exptarget = fs::lookup(inode, pathname, options);
  }
  else
  {
    xDeb("newfstatat with negative fd and relative path");
    return -1;
  }

  if (!exptarget)
  {
    xDeb("Can't get target inode");
    return -1;
  }

  auto target = *exptarget;

  *buf = {};
  buf->st_nlink = 1;
  buf->st_size = target->i_size;
  buf->st_mode = target->i_mode;
  buf->st_blksize = 1;
  buf->st_blocks = (target->i_size + 511) / 512;

  return 0;
}

long clone(const InterruptState& st,
    unsigned long flags,
    void* child_stack,
    void* ptid,
    void* ctid,
    struct pt_regs* regs)
{
  xDeb("clone(%#016x, %p, %p, %p, %p)", flags, child_stack, ptid, ctid, regs);

  if (ptid || regs)
  {
    xErr("unsupported arguments ptid or regs");
    return -1;
  }

  return TaskManager::get()->clone(st);
}

int execve(const char* filename, const char* argv[], const char* envp[])
{
  xDeb("execve(\"%s\", %p, %p)", filename, argv, envp);

  if (filename[0] != '/')
  {
    xErr("execve with relative path is not implemented");
    return -1;
  }

  auto exptarget = fs::lookup(nullptr, filename, fs::LookupOptions_None);
  if (!exptarget)
  {
    xDeb("Can't get exec target inode");
    return -1;
  }

  auto exphandle = (*exptarget)->open();
  if (!exphandle)
  {
    xDeb("Can't open exec target");
    return -1;
  }

  PageDirectory::getCurrent()->unmapUserSpace();

  elf::exec(**exphandle);

  // something failed
  return -1;
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

#include "syscalls/syscall_register.hxx"
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
  xDeb("Syscall %d, from ip %x, sp %x", s->rax, s->rip, s->rsp);
  s->rax = sys::handle(*s);
}
