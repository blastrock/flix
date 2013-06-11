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
    Dmesg();
    ~Dmesg();

    template <typename T>
    Dmesg& operator<<(const T& value);

  private:
    typedef ForwardAllocator<char, StaticMemoryPool> Allocator;
    typedef std::basic_string<char, std::char_traits<char>, Allocator> String;
    typedef std::basic_ostringstream<String::value_type, String::traits_type,
            String::allocator_type> Stream;

    char m_buf[100];
    StaticMemoryPool m_pool;
    String m_str;
    Stream m_ss;
};

inline Dmesg::Dmesg() :
  m_pool(m_buf, sizeof(m_buf)),
  m_str(Allocator(&m_pool)),
  m_ss(m_str)
{
}

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
