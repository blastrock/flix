#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>
#include <set>

#include "Mutex.hpp"
#include "CondVar.hpp"
#include "PageDirectory.hpp"
#include "FileManager.hpp"

struct InterruptState;

struct Task
{
  struct Context
  {
    // callee saved
    uint64_t r15, r14, r13, r12, rbx, rbp;
    // caller saved
    uint64_t r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi;
    uint64_t rip, cs, rflags, rsp, ss;
  } __attribute__((packed));

  enum State
  {
    Runnable,
    Sleeping,
    Zombie,
  };

  struct StateHolder
  {
    State state;
    Mutex stateMutex;
    CondVar stateCond;

    StateHolder() = default;
    StateHolder(StateHolder&& r)
      : state(r.state)
    {}
  };

  // this is a threadsafe structure, locks are always taken from top to bottom
  // and then from left to right to avoid deadlocks
  struct HierarchyHolder
  {
    Mutex mutex;
    Task* parent = nullptr;
    Task* firstChild = nullptr;
    Task* nextSibling = nullptr;
    Task* prevSibling = nullptr;

    HierarchyHolder() = default;
    HierarchyHolder(HierarchyHolder&& r)
      : parent(r.parent)
      , firstChild(r.firstChild)
      , nextSibling(r.nextSibling)
    {
      r.parent = nullptr;
    }

    ~HierarchyHolder()
    {
      // was moved out
      if (!parent)
        return;

      std::vector<decltype(mutex.getScoped())> locks;

      locks.emplace_back(parent->hh.mutex.getScoped());

      if (prevSibling)
        locks.emplace_back(prevSibling->hh.mutex.getScoped());

      locks.emplace_back(mutex.getScoped());

      if (nextSibling)
        locks.emplace_back(nextSibling->hh.mutex.getScoped());

      for (auto* child = firstChild; child; child = child->hh.nextSibling)
        locks.emplace_back(child->hh.mutex.getScoped());

      if (&parent->hh.firstChild->hh == this)
        parent->hh.firstChild = nextSibling;
      if (prevSibling)
        prevSibling->hh.nextSibling = nextSibling;
      if (nextSibling)
        nextSibling->hh.prevSibling = prevSibling;

      assert(!firstChild && "making orphans is not implemented");
    }

    void addChild(Task& child)
    {
      std::vector<decltype(mutex.getScoped())> locks;
      locks.reserve(3);
      locks.emplace_back(child.hh.mutex.getScoped());
      locks.emplace_back(mutex.getScoped());
      if (firstChild)
      {
        locks.emplace_back(firstChild->hh.mutex.getScoped());

        assert(!firstChild->hh.prevSibling);
        firstChild->hh.prevSibling = &child;
      }

      assert(!child.hh.prevSibling);
      child.hh.nextSibling = firstChild;

      firstChild = &child;

      assert(child.hh.parent);
    }
  };

  // TODO try to mark this const
  pid_t tid;
  HierarchyHolder hh;

  std::function<void(Task&)> releaseFunction;

  StateHolder sh;

  Context context;
  PageDirectory pageDirectory;

  char* stack;
  char* stackTop;

  char* kernelStack;
  char* kernelStackTop;

  FileManager fileManager;

  Task(Task&&) = default;
  Task& operator=(Task&&) = default;
  ~Task();
};

struct TaskComparator
{
  using is_transparent = void;

  bool operator()(const Task& t1, const Task& t2)
  {
    return t1.tid < t2.tid;
  }
  template <typename T>
  bool operator()(const T& t1, const Task& t2)
  {
    return t1 < t2.tid;
  }
  template <typename T>
  bool operator()(const Task& t1, const T& t2)
  {
    return t1.tid < t2;
  }
};

struct TaskStateSegment
{
  typedef void* Ptr;

  uint32_t reserved0;
  Ptr rsp0, rsp1, rsp2;
  uint64_t reserved1;
  Ptr ist1, ist2, ist3, ist4, ist5, ist6, ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t iomap;
} __attribute__((packed));

class TaskManager
{
public:
  static TaskManager* get();

  TaskManager();

  void setUpTss();

  pid_t addTask(Task&& t);
  Task newKernelTask();
  Task newUserTask();
  void downgradeCurrentTask();
  [[noreturn]] void terminateCurrentTask();
  pid_t wait(pid_t tid, int* status);

  pid_t clone(const InterruptState& st);

  void prepareMeForSleep();
  void putMeToSleep();
  void wakeUpTask(Task& task);

  void saveCurrentTask(const Task::Context& t);
  [[noreturn]] void scheduleNext();
  [[noreturn]] void rescheduleSelf();

  bool isTaskActive();
  /**
   * Get the active task or 0 if kernel was sleeping
   */
  Task* getActive();
  /**
   * Get the active task.
   *
   * If there is no active task, the behavior is undefined.
   */
  Task& getActiveTask();
  /**
   * Get the active TID.
   *
   * This method does not lock and is safe to use in logging contexts to avoid
   * re-entrance.
   *
   * \return the TID of the active task, 0 if there is no such task.
   */
  pid_t getActiveTid();
  /**
   * Get the task corresponding to \p tid if it exists
   */
  Task* getTask(pid_t tid);

private:
  using Tasks = std::set<Task, TaskComparator>;

  static TaskManager* instance;

  TaskStateSegment* _tss = nullptr;

  std::set<Task, TaskComparator> _tasks;
  pid_t _activeTask = 0;
  pid_t _nextTid = 1;

  void setKernelStack();
  void updateNextTid();
  void tryScheduleNext();
  Tasks::iterator getNext();
  [[noreturn]] void enterSleep();
  void doInterruptMasking();
};

#endif /* TASK_HPP */
