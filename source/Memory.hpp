#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

#include "Types.hpp"

class Memory
{
public:
  static Memory& get();

  page_t getFreePage();
  void setPageFree(page_t page);
  void setPageUsed(page_t page);
  void setRangeUsed(page_t from, page_t to);
  void completeRangeUsed(page_t from, page_t to);

private:
  std::vector<bool> _frames;
};

#endif /* MEMORY_HPP */
