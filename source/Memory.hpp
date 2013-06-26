#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

class Memory
{
  public:
    static uint64_t getFreePage();
    static void setPageFree(uint64_t page);
    static void setPageUsed(uint64_t page);

  private:
    static std::vector<bool> g_frames;
};

#endif /* MEMORY_HPP */
