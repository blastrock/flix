#ifndef STATIC_MEMORY_POOL_HPP
#define STATIC_MEMORY_POOL_HPP

#include <cstdint>

class StaticMemoryPool
{
  public:
    StaticMemoryPool(void* pool, uint64_t size);

    void* allocate(uint64_t size);

  private:
    uint8_t* m_start;
    uint8_t* m_ptr;
    uint8_t* m_end;
};

#endif /* STATIC_MEMORY_POOL_HPP */
