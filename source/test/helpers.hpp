#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"

namespace th
{

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
void runTest(const char* name, F&& func)
{
  {
    std::ostringstream ss;
    xll::pnt::writef(*ss.rdbuf(), "\n[BEGIN TEST \"%s\"]\n", name);
    printE9(ss.str().c_str());
  }

  func();

  printE9("\n[END TEST]\n");
}

void fail()
{
  printE9("\n[FAIL]\n");
}

void finish()
{
  printE9("\n[ALL TESTS RUN]\n");
}

}
