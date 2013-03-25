#ifndef MEMORY_HPP
#define MEMORY_HPP

#include "BitVector.hpp"
#include "Multiboot.hpp"

class Memory
{
  public:
    static void init(Multiboot const& mboot);

  private:
    static BitVector g_frames;

    template <typename T>
    static void forEachRange(Multiboot const& mboot, T func);
};

#endif /* MEMORY_HPP */
