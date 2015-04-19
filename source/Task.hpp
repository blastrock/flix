#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>
#include <vector>

struct Task
{
  // callee saved
  uint64_t r15, r14, r13, r12, rbx, rbp;
  // caller saved
  uint64_t r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi;
  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

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

  void addTask(const Task& t);
  Task newKernelTask();

  void saveCurrentTask(const Task& t);
  void scheduleNext();

private:
  static TaskManager* instance;

  std::vector<Task> _tasks;
  unsigned int _currentTask;
};

#endif /* TASK_HPP */
