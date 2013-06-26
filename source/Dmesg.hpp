#ifndef DMESG_HPP
#define DMESG_HPP

#include <sstream>
#include <cstring>
#include "Screen.hpp"
#include "ConsoleStreamBuf.hpp"

class Dmesg
{
  public:
    Dmesg();
    ~Dmesg();

    template <typename T>
    Dmesg& operator<<(const T& value);

  private:
    ConsoleStreamBuf m_sb;
    std::ostream m_stream;
};

inline Dmesg::Dmesg() :
  m_stream(&m_sb)
{
}

inline Dmesg::~Dmesg()
{
  Screen::putChar('\n');
}

template <typename T>
Dmesg& Dmesg::operator<<(const T& value)
{
  m_stream << value;
  return *this;
}

#endif /* DMESG_HPP */
