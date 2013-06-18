#ifndef MEMORY_HPP
#define MEMORY_HPP

//#include "BitVector.hpp"
//#include "Multiboot.hpp"
#include <vector>

class Memory
{
  public:
    static void init();
    //static void init(Multiboot const& mboot);

    static uint64_t getFreePage();
    static void setPageFree(uint64_t page);

  private:
    static std::vector<bool> g_frames;

    //template <typename T>
    //static void forEachRange(Multiboot const& mboot, T func);
};

#endif /* MEMORY_HPP */
