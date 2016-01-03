#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"
#include "Timer.hpp"
#include "helpers.hpp"

XLL_LOG_CATEGORY("main");

void disableIrq(volatile bool& updating)
{
  for (int i = 0; i < 50; ++i)
  {
    DisableInterrupts _;
    if (updating)
      th::fail();
    updating = true;
    for (int i = 0; i < 10; ++i)
      asm volatile ("");
    updating = false;
  }
}

template <typename L>
void lockAndUpdate(L& mutex, volatile bool& updating)
{
  for (int i = 0; i < 50; ++i)
  {
    auto lock = mutex.getScoped();
    if (updating)
      th::fail();
    updating = true;
    for (int i = 0; i < 10; ++i)
      asm volatile ("");
    updating = false;
  }
}

void waitEnd()
{
  {
    volatile bool updating = false;
    auto lockUpdateFunc = [&]{ disableIrq(updating); };

    th::runTest("disable_irq_simple", lockUpdateFunc);
    th::runTestProcesses("disable_irq_concurrent",
        lockUpdateFunc,
        lockUpdateFunc,
        lockUpdateFunc);
  }
  {
    SpinLock spinlock;
    volatile bool updating = false;
    auto lockUpdateFunc = [&]{ lockAndUpdate(spinlock, updating); };

    th::runTest("spinlock_simple_lock", lockUpdateFunc);
    th::runTestProcesses("spinlock_concurrent_lock",
        lockUpdateFunc,
        lockUpdateFunc,
        lockUpdateFunc);
  }
  {
    Mutex mutex;
    volatile bool updating = false;
    auto lockUpdateFunc = [&]{ lockAndUpdate(mutex, updating); };

    th::runTest("mutex_simple_lock", lockUpdateFunc);
    th::runTestProcesses("mutex_concurrent_lock",
        lockUpdateFunc,
        lockUpdateFunc,
        lockUpdateFunc);
  }

  th::finish();
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Timer::init(1000);

  auto& taskManager = *TaskManager::get();

  {
    Task task = taskManager.newKernelTask();
    task.stack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&waitEnd);
    taskManager.addTask(std::move(task));
  }

  xInf("End of kernel");

  taskManager.scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
