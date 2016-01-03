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
pid_t startTask(F&& func)
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
  return taskManager->addTask(std::move(task));
}

template <typename... F>
void runTestProcesses(const char* name, F&&... funcs)
{
  {
    std::ostringstream ss;
    xll::pnt::writef(*ss.rdbuf(), "\n[BEGIN TEST \"%s\"]\n", name);
    printE9(ss.str().c_str());
  }

  auto pids = {startTask([funcs = std::forward<F>(funcs)]{
        funcs();
        sys::call(sys::exit);
      })...};
  for (const auto& pid : pids)
    sys::call(sys::wait4, pid, nullptr, 0, nullptr);

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
