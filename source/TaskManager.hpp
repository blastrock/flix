#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>
#include <set>

#include "PageDirectory.hpp"
#include "FileManager.hpp"

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

  // TODO try to mark this const
  pid_t tid;
  Context context;
  PageDirectory pageDirectory;
  char* stack;
  char* stackTop;
  FileManager fileManager;
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

  static void setUpTss();

  void addTask(Task&& t);
  Task newKernelTask();
  Task newUserTask();
  void downgradeCurrentTask();
  void terminateCurrentTask();

  void saveCurrentTask(const Task::Context& t);
  [[noreturn]] void scheduleNext();
  [[noreturn]] void rescheduleSelf();

  Task& getActiveTask();

private:
  static TaskManager* instance;

  std::set<Task, TaskComparator> _tasks;
  pid_t _activeTask;
  pid_t _nextTid;

  void updateNextTid();
};

#endif /* TASK_HPP */
