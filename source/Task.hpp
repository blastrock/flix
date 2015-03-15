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

class TaskManager
{
public:
  static TaskManager* get();

  TaskManager();

  void addTask(const Task& t);

  void saveCurrentTask(const Task& t);
  void scheduleNext();

private:
  static TaskManager* instance;

  std::vector<Task> _tasks;
  unsigned int _currentTask;
};

#endif /* TASK_HPP */
