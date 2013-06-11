#include "StaticMemoryPool.hpp"

StaticMemoryPool::StaticMemoryPool() :
  m_start(nullptr),
  m_ptr(nullptr),
  m_end(nullptr)
{
}

StaticMemoryPool::StaticMemoryPool(void* pool, uint64_t size)
{
  set(pool, size);
}

void StaticMemoryPool::set(void* pool, uint64_t size)
{
  m_start = static_cast<uint8_t*>(pool);
  m_ptr = m_start;
  m_end = m_start + size;
}

void* StaticMemoryPool::allocate(uint64_t size)
{
  if (m_ptr + size > m_end)
    return nullptr;

  void* block = m_ptr;
  m_ptr += size;

  return block;
}
