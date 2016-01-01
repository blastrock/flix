#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Screen.hpp"
#include "Syscall.hpp"
#include "Symbols.hpp"
#include "Elf.hpp"

XLL_LOG_CATEGORY("main");

Mutex* mutex;
volatile bool updating = false;

void lockAndUpdate()
{
  for (int i = 0; i < 50; ++i)
  {
    auto lock = mutex->getScoped();
    if (updating)
      printE9("\n[FAIL]\n");
    updating = true;
    updating = false;
  }
}

void runTask(std::function<void()>* pf)
{
  auto f = std::move(*pf);
  delete pf;
  f();
}

template <typename F>
void startTask(F&& func)
{
  auto taskManager = TaskManager::get();
  Task task = taskManager->newKernelTask();
  task.stack = static_cast<char*>(getStackPageHeap().kmalloc().first);
  task.stackTop = task.stack + 0x4000;
  task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
  task.kernelStackTop = task.kernelStack + 0x4000;
  task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
  task.context.rip = reinterpret_cast<uint64_t>(&runTask);
  task.context.rdi = reinterpret_cast<uint64_t>(
      new std::function<void()>(std::forward<F>(func)));
  taskManager->addTask(std::move(task));
}

template <std::size_t... I, typename... F, typename A>
void startVariadic(
    A& vec, std::index_sequence<I...>, F&&... funcs)
{
  (void)std::initializer_list<int>{(
      startTask([funcs = std::forward<F>(funcs), &vec]{
          funcs();
          vec[I] = true;
          sys::call(sys::exit);
      }), 0)...};
}

template <typename... F>
void runTestProcesses(const char* name, F&&... funcs)
{
  // must be on the heap to be shared with everybody
  auto finished =
    std::make_unique<std::array<volatile bool, sizeof...(funcs)>>();

  {
    std::ostringstream ss;
    xll::pnt::writef(*ss.rdbuf(), "\n[BEGIN TEST \"%s\"]\n", name);
    printE9(ss.str().c_str());
  }

  startVariadic(
      *finished, std::index_sequence_for<F...>(), std::forward<F>(funcs)...);

  for (const auto& finish : *finished)
    while (!finish)
      ;
  printE9("\n[END TEST]\n");
}

template <typename F>
void runTestProcess(const char* name, F&& func)
{
  {
    std::ostringstream ss;
    xll::pnt::writef(*ss.rdbuf(), "\n[BEGIN TEST \"%s\"]\n", name);
    printE9(ss.str().c_str());
  }

  func();

  printE9("\n[END TEST]\n");
}

void waitEnd()
{
  mutex = new Mutex;

  runTestProcess("mutex_simple_lock", lockAndUpdate);

  runTestProcesses("mutex_concurrent_lock", lockAndUpdate, lockAndUpdate);

  printE9("\n[ALL TESTS RUN]\n");
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::clear();
  Screen::putString("Booting Flix\n");

  auto taskManager = TaskManager::get();

  {
    Task task = taskManager->newKernelTask();
    task.stack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&waitEnd);
    taskManager->addTask(std::move(task));
  }

  xInf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
