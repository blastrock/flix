#ifndef DMESG_HPP
#define DMESG_HPP

#include <sstream>
#include <cstring>
#include "Screen.hpp"
#include "StaticMemoryPool.hpp"
#include "ForwardAllocator.hpp"

class Dmesg
{
  public:
    ~Dmesg();

    template <typename T>
    Dmesg& operator<<(const T& value);

  private:
    std::ostringstream m_ss;
};

inline Dmesg::~Dmesg()
{
  Screen::putChar('\n');
}

template <typename T>
Dmesg& Dmesg::operator<<(const T& value)
{
  m_ss << value;
  Screen::putString(m_ss.str().c_str());
  m_ss.str("");
  return *this;
}

#endif /* DMESG_HPP */
