#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

#include "Types.hpp"

class Memory
{
  public:
    static page_t getFreePage();
    static void setPageFree(page_t page);
    static void setPageUsed(page_t page);
    static void setRangeUsed(page_t from, page_t to);
    static void completeRangeUsed(page_t from, page_t to);

  private:
    static std::vector<bool> g_frames;
};

#endif /* MEMORY_HPP */
