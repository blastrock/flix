#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

class Memory
{
  public:
    static uintptr_t getFreePage();
    static void setPageFree(uintptr_t page);
    static void setPageUsed(uintptr_t page);

  private:
    static std::vector<bool> g_frames;
};

#endif /* MEMORY_HPP */
