#include "TaskManager.hpp"
#include "Cpu.hpp"
#include "Interrupt.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "DescTables.hpp"
#include "Util.hpp"

XLL_LOG_CATEGORY("core/taskmanager");

TaskManager* TaskManager::instance;

[[noreturn]] extern "C" void jump_to_task(Task::Context* task);
[[noreturn]] extern "C" void jump_to_address(
    void* addr, void* arg1, void* stack);
extern "C" int task_save(Task::Context* task);

Task::~Task()
{
  // if pageDirectory is invalid, it means the task has been moved out
  if (pageDirectory.isValid())
  {
    if (releaseFunction)
      releaseFunction(*this);
    pageDirectory.unmapUserSpace();
  }
}

TaskManager* TaskManager::get()
{
  if (!instance)
    instance = new TaskManager;
  return instance;
}

TaskManager::TaskManager()
{
  // for the moment there are 0 task, so we don't need to tick
  Interrupt::mask(IRQ_TIMER);
}

void TaskManager::setUpTss()
{
  _tss = new TaskStateSegment{};
  DescTables::initTr(_tss);

  setKernelStack();
}

void TaskManager::updateNextTid()
{
  // TODO handle the case where the set is full
  while (_tasks.find(_nextTid) != _tasks.end())
    ++_nextTid;
}

pid_t TaskManager::addTask(Task&& t)
{
  DisableInterrupts _;

  updateNextTid();

  t.tid = _nextTid;
  ++_nextTid;

  xDeb("Adding new task with tid %d", t.tid);
  auto ret = _tasks.insert(std::move(t));
  assert(ret.second);
  auto& newTask = const_cast<Task&>(*ret.first);

  if (t.tid > 1)
    if (newTask.hh.parent)
      newTask.hh.parent->hh.addChild(newTask);
    else
    {
      auto& activeTask = getActiveTask();
      newTask.hh.parent = &activeTask;
      activeTask.hh.addChild(newTask);
    }
  else
    assert(!t.hh.parent && "task 1 must have no parent");

  if (t.sh.state == Task::State::Runnable)
    doInterruptMasking();

  return t.tid;
}

pid_t TaskManager::clone(const InterruptState& st)
{
  xDeb("Cloning task");

  Task task = newUserTask();
  task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
  task.kernelStackTop = task.kernelStack + 0x4000;
  task.releaseFunction = [](auto& task){
    getStackPageHeap().kfree(task.kernelStack);
  };
  auto& newPd = task.pageDirectory;

  // TODO random address, use something less arbitrary
  auto bufferPage = reinterpret_cast<char*>(0xffffffffaa000000);

  xDeb("Cloning memory");
  auto& activeTask = getActiveTask();
  auto& pd = activeTask.pageDirectory;
  pd.forEachUserPage(
      [&](const PageDirectory::PageTableEntry& entry, void* addr, physaddr_t)
      {
        xDeb("Cloning %p, %x", addr, entry.getAttributes());

        physaddr_t physaddr;
        newPd.mapPage(addr, entry.getAttributes(), &physaddr);

        if (entry.getAttributes() & PageDirectory::ATTR_DEFER)
          return;

        pd.mapPageTo(bufferPage,
            physaddr,
            PageDirectory::ATTR_RW | PageDirectory::ATTR_NOEXEC);
        memcpy(bufferPage, addr, PAGE_SIZE);
        pd.unmapPage(bufferPage);
      });

  task.sh.state = Task::State::Runnable;
  task.context = st.toTaskContext();
  task.context.rax = 0;

  addTask(std::move(task));

  return task.tid;
}

void TaskManager::terminateCurrentTask()
{
  xDeb("Terminating task %d, size %d", _activeTask, _tasks.size());

  assert(!(Cpu::rflags() & (1 << 9)));

  auto& task = getActiveTask();

  if (task.tid == 1)
    PANIC("PID 1 is dead");

  DisableInterrupts _;
  _activeTask = 0;

  {
    auto lock = task.sh.stateMutex.getScoped();
    task.sh.state = Task::State::Zombie;
    task.sh.stateCond.notify_one();
  }

  doInterruptMasking();

  scheduleNext();
}

pid_t TaskManager::wait(pid_t tid, int* status)
{
  xDeb("Waiting on %d", tid);

  if (tid == static_cast<pid_t>(-1))
  {
    // TODO wait on all children
    xWar("wait(-1) is partially implemented, waiting only on first child");

    auto& task = getActiveTask();
    auto lock = task.hh.mutex.getScoped();
    if (!task.hh.firstChild)
    {
      xDeb("No child to wait");
      return -1;
    }
    tid = task.hh.firstChild->tid;
    xDeb("Will wait on %d", tid);
  }

  auto task = getTask(tid);
  if (!task)
  {
    xDeb("Invalid tid");
    return -1;
  }

  // FIXME lock?
  if (task->hh.parent->tid != _activeTask)
  {
    xDeb("Waiting on a non-child");
    return -1;
  }

  {
    auto lock = task->sh.stateMutex.getScoped();
    if (task->sh.state != Task::State::Zombie)
      task->sh.stateCond.wait(task->sh.stateMutex);
  }

  DisableInterrupts _;
  xDeb("Wait finished, removing task");
  const auto iter = _tasks.find(tid);
  assert(iter != _tasks.end());
  _tasks.erase(iter);

  if (status)
    *status = 0;

  return tid;
}

Task TaskManager::newKernelTask()
{
  Task task{};
  task.context.cs = DescTables::SYSTEM_CS;
  task.context.ss = DescTables::SYSTEM_DS;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

Task TaskManager::newUserTask()
{
  Task task{};
  task.context.cs = DescTables::USER_CS | 0x3;
  task.context.ss = DescTables::USER_DS | 0x3;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

void TaskManager::downgradeCurrentTask()
{
  Task& task = getActiveTask();
  task.context.cs = DescTables::USER_CS | 0x3;
  task.context.ss = DescTables::USER_DS | 0x3;
}

void TaskManager::saveCurrentTask(const Task::Context& ctx)
{
  DisableInterrupts _;

  xDeb("Saving task %d with rip %x and rsp %x", _activeTask, ctx.rip, ctx.rsp);
  getActiveTask().context = ctx;

  assert((ctx.cs == DescTables::SYSTEM_CS ||
        (ctx.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
}

void TaskManager::prepareMeForSleep()
{
  Task& task = getActiveTask();
  task.sh.state = Task::State::Sleeping;

  doInterruptMasking();
}

void TaskManager::putMeToSleep()
{
  xDeb("Putting task to sleep");

  assert(SpinLock::getLockCount() == 0 &&
         "no spinlock must be taken while going to sleep");

  Task& task = getActiveTask();

  DisableInterrupts _;
  if (task.sh.state != Task::State::Sleeping)
  {
    xDeb("Task has been woken up before going to sleep");
    return;
  }

  if (!task_save(&task.context))
  {
    xDeb("Task rip:%x rsp:%x", task.context.rip, task.context.rsp);

    assert(task.context.cs == DescTables::SYSTEM_CS &&
        "putMeToSleep called from userspace");

    scheduleNext(); // going to sleep
  }
  else
    return; // waking up
}

void TaskManager::wakeUpTask(Task& task)
{
  task.sh.state = Task::State::Runnable;

  doInterruptMasking();
}

bool TaskManager::isTaskActive()
{
  return _activeTask;
}

Task* TaskManager::getActive()
{
  DisableInterrupts _;
  if (_activeTask == 0)
    return nullptr;
  return getTask(_activeTask);
}

Task* TaskManager::getTask(pid_t tid)
{
  DisableInterrupts _;
  const auto iter = _tasks.find(tid);
  if (iter == _tasks.end())
    return nullptr;
  return const_cast<Task*>(&*iter);
}

Task& TaskManager::getActiveTask()
{
  DisableInterrupts _;
  const auto task = getActive();
  assert(task && "there is an active task");
  return *task;
}

pid_t TaskManager::getActiveTid()
{
  return _activeTask;
}

TaskManager::Tasks::iterator TaskManager::getNext()
{
  auto id = _activeTask;
  bool looped = false;
  while (true)
  {
    auto iter = _tasks.upper_bound(id);
    if (iter == _tasks.end())
    {
      if (looped)
        return _tasks.end();

      id = 0;
      looped = true;
      continue;
    }
    if (iter->sh.state == Task::State::Runnable)
      return iter;
    ++id;
  }
}

void TaskManager::setKernelStack()
{
  auto kernelStack = Symbols::getStackBase();

  _tss->ist1 = kernelStack;
  Cpu::setKernelStack(kernelStack);
}

void TaskManager::enterSleep()
{
  _activeTask = 0;

  setKernelStack();

  enableInterrupts();
  asm volatile("hlt":::"memory");

  PANIC("returned from hlt in enterSleep");
}

void TaskManager::tryScheduleNext()
{
  assert(!std::all_of(_tasks.begin(), _tasks.end(), [](const auto& task){
          return task.sh.state == Task::State::Zombie;
        }));

  const auto iter = getNext();
  if (iter == _tasks.end())
    return;

  Task& nextTask = const_cast<Task&>(*iter);
  _activeTask = nextTask.tid;
  assert((nextTask.context.cs == DescTables::SYSTEM_CS ||
        (nextTask.context.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
  xDeb("Restoring task %d with rip %x and rsp %x (int: %s)", _activeTask,
      nextTask.context.rip, nextTask.context.rsp,
      !!(nextTask.context.rflags & (1 << 9)));

  // set the page directory of the process
  nextTask.pageDirectory.use();

  // set the kernel stack for interrupt/syscall handling
  _tss->ist1 = nextTask.kernelStackTop;
  Cpu::setKernelStack(nextTask.kernelStackTop);

  jump_to_task(&nextTask.context);
}

void TaskManager::scheduleNext()
{
  xDeb("Schedule next");
  tryScheduleNext();
  xDeb("All processes sleeping, entering kernel sleep");
  enterSleep();
}

void TaskManager::rescheduleSelf()
{
  assert(!_tasks.empty());

  Task& nextTask = getActiveTask();
  assert((nextTask.context.cs == DescTables::SYSTEM_CS ||
        (nextTask.context.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
  xDeb("Rescheduling self at %x", nextTask.context.rip);
  jump_to_task(&nextTask.context);
}

void TaskManager::doInterruptMasking()
{
  DisableInterrupts _;

  bool foundFirst = false;

  for (const auto& task : _tasks)
    if (task.sh.state == Task::Runnable)
    {
      if (foundFirst)
      {
        xDeb("Ticking enabled");
        Interrupt::unmask(IRQ_TIMER);
        return;
      }
      else
        foundFirst = true;
    }

  xDeb("Ticking disabled");
  Interrupt::mask(IRQ_TIMER);
}
