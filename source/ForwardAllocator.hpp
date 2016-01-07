#ifndef FORWARD_ALLOCATOR_HPP
#define FORWARD_ALLOCATOR_HPP

#include <cstdint>

template <typename T, typename Pool>
class ForwardAllocator
{
  public:
    typedef T value_type;

    explicit ForwardAllocator(Pool* pool = nullptr);
    template <typename T2>
    ForwardAllocator(const ForwardAllocator<T2, Pool>& o);
    template <typename T2>
    ForwardAllocator(ForwardAllocator<T2, Pool>&& o);

    T* allocate(std::size_t size);
    void deallocate(T* p, std::size_t size);

    bool operator==(const ForwardAllocator<T, Pool>& other);
    bool operator!=(const ForwardAllocator<T, Pool>& other);

  private:
    Pool* m_pool;
};

template <typename T, typename Pool>
ForwardAllocator<T, Pool>::ForwardAllocator(Pool* pool) :
  m_pool(pool)
{
}

template <typename T, typename Pool>
template <typename T2>
ForwardAllocator<T, Pool>::ForwardAllocator(const ForwardAllocator<T2, Pool>& o) :
  m_pool(o.m_pool)
{
}

template <typename T, typename Pool>
template <typename T2>
ForwardAllocator<T, Pool>::ForwardAllocator(ForwardAllocator<T2, Pool>&& o) :
  m_pool(o.m_pool)
{
  o.m_pool = nullptr;
}

template <typename T, typename Pool>
T* ForwardAllocator<T, Pool>::allocate(std::size_t nb)
{
  if (!m_pool)
    return nullptr;

  std::size_t size = nb * sizeof(T);

  return static_cast<T*>(m_pool->allocate(size));
}

template <typename T, typename Pool>
void ForwardAllocator<T, Pool>::deallocate(T*, std::size_t)
{
}

template <typename T, typename Pool>
bool ForwardAllocator<T, Pool>::operator==(const ForwardAllocator<T, Pool>& other)
{
  return m_pool == other.m_pool;
}

template <typename T, typename Pool>
bool ForwardAllocator<T, Pool>::operator!=(const ForwardAllocator<T, Pool>& other)
{
  return !(*this == other);
}

#endif /* FORWARD_ALLOCATOR_HPP */
