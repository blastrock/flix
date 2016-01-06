#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Screen.hpp"
#include "Syscall.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include "Elf.hpp"

XLL_LOG_CATEGORY("main");

void exec()
{
  xDeb("Opening file");
  auto einode = fs::lookup("/bin/sh");
  if (!einode)
  {
    xErr("failed to open file");
    sys::call(sys::exit);
  }
  auto ehndl = (*einode)->open();
  std::vector<std::string> args = {"sh"};
  xDeb("Execing");
  // FIXME exec never returns so hndl and args leak
  elf::exec(**ehndl, args);

  PANIC("init exec failed");
}

void forkmaster()
{
  for (int i = 0; i < 10; ++i)
  {
    xDeb("Used memory: %d", Memory::get().getUsedPageCount());
    xDeb("Used page heap blocks: %d", getPdPageHeap().getUsedBlockCount());
    xDeb("Used stack blocks: %d", getStackPageHeap().getUsedBlockCount());
    int ret = sys::call(sys::clone, 0, nullptr, nullptr, nullptr, nullptr);
    if (ret < -1)
      xFat("FAIL");
    else if (ret == 0)
      sys::call(sys::exit);
    else
      sys::call(sys::wait4, ret, nullptr, 0, nullptr);
  }
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::clear();
  Screen::putString("Booting Flix\n");

  auto taskManager = TaskManager::get();

  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&exec);
    taskManager->addTask(std::move(task));
  }
#if 0
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0x00000000a0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&forkmaster);
    taskManager->addTask(std::move(task));
  }
#endif

  xInf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
