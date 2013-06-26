#ifndef CONSOLE_STREAM_BUF_HPP
#define CONSOLE_STREAM_BUF_HPP

#include <streambuf>

class ConsoleStreamBuf : public std::streambuf
{
  protected:
    int_type overflow(int_type ch)
    {
      if (!traits_type::eq_int_type(ch, traits_type::eof()))
      {
        sync();
        Screen::putChar(ch);
        return ch;
      }
      return traits_type::eof();
    }

    int sync()
    {
      for (char_type* ptr = pbase(); ptr < pptr(); ++ptr)
        Screen::putChar(*ptr);
      pbump(pptr() - pbase());

      return 0;
    }
};

#endif /* CONSOLE_STREAM_BUF_HPP */
