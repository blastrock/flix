// Copyright (c) 2013, Philippe Daouadi <p.daouadi@free.fr>
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met: 
// 
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// 
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of the FreeBSD Project.

#ifndef PNT_HPP
#define PNT_HPP

#include <cassert>
#include <limits>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef FORMATTER_THROW_ON_ERROR
#define FORMAT_ERROR(type) \
  throw FormatError(type)
#else
#define FORMAT_ERROR(type) \
  assert(!#type)
#endif

namespace pnt
{

class FormatError : public std::exception
{
  public:
    enum Type
    {
      InvalidFormatter,
      TooFewArguments,
      TooManyArguments,
      IncompatibleType,
      NotImplemented
    };

    FormatError(Type type);

    const char* what() const noexcept;

  private:
    Type m_type;
};

inline FormatError::FormatError(Type type) :
  m_type(type)
{
}

inline const char* FormatError::what() const noexcept
{
  switch (m_type)
  {
    case InvalidFormatter: return "Invalid formatter";
    case TooFewArguments: return "Too few arguments";
    case TooManyArguments: return "Too many arguments";
    case IncompatibleType: return "Incompatible type";
    case NotImplemented: return "Not implemented";
    default: return "Unknown error";
  }
}

namespace _Formatter
{
  template <typename T>
  struct is_integral
  {
    static constexpr bool value =
      std::is_integral<T>::value &&
      !std::is_same<T, bool>::value;
  };

  template <typename...>
  struct sfinae_true : std::true_type {};

  namespace detail
  {
    template<typename T>
    static sfinae_true<decltype(std::declval<T>().begin()),
        decltype(std::declval<T>().end())> test_iterable(int);
    template<typename>
    static std::false_type test_iterable(long);
  }

  template<class T>
  struct is_iterable : decltype(detail::test_iterable<T>(0)){};
}

namespace _Formatter
{
  template <typename Iterator>
  class FormatterItem
  {
    public:
      static constexpr unsigned int POSITION_NONE = -1;

      static constexpr unsigned int FLAG_LEFT_JUSTIFY  =  0x1;
      static constexpr unsigned int FLAG_SHOW_SIGN     =  0x2;
      static constexpr unsigned int FLAG_EXPLICIT_BASE =  0x4;
      static constexpr unsigned int FLAG_FILL_ZERO     =  0x8;
      static constexpr unsigned int FLAG_ADD_SPACE     = 0x10;

      static constexpr unsigned int WIDTH_EMPTY = -1;
      static constexpr unsigned int WIDTH_ARG = -2;

      Iterator ctnBegin;
      Iterator ctnEnd;

      unsigned int position;
      unsigned char flags;
      unsigned int width;
      unsigned int precision;
      char formatChar;

      void fixFlags();

      void handleFormatter(Iterator& iter);

    private:
      Iterator findIntegerEnd(Iterator iter);
      unsigned int parseInt(Iterator iter);

      void handlePosition(Iterator& iter);
      void handleFlags(Iterator& iter);
      void handleWidth(Iterator& iter);
      void handlePrecision(Iterator& iter);
      void handleFormatChar(Iterator& iter);
  };

  template <typename Iterator>
  inline void FormatterItem<Iterator>::fixFlags()
  {
    switch (formatChar)
    {
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
        // if precision is undefined for float, default to 6
        if (precision == WIDTH_EMPTY)
          precision = 6;
        break;
      case 'x':
      case 'X':
      case 'o':
        // no sign or space for non decimal and non binary and non %s
        flags &= ~(FLAG_SHOW_SIGN | FLAG_ADD_SPACE);
        break;
      case 'c':
      case 'd':
      case 'b':
        // no explicit base for decimal, binary, char
        flags &= ~FLAG_EXPLICIT_BASE;
        break;
    }

    // no space if sign
    if (flags & FLAG_SHOW_SIGN)
      flags &= ~FLAG_ADD_SPACE;

    // fill only with space on left justify
    if (flags & FLAG_LEFT_JUSTIFY)
      flags &= ~FLAG_FILL_ZERO;
  }

  template <typename Iterator>
  inline Iterator FormatterItem<Iterator>::findIntegerEnd(
      Iterator iter)
  {
    auto end = iter;
    while (*end >= '0' && *end <= '9')
      ++end;
    return end;
  }

  template <typename Iterator>
  inline unsigned int FormatterItem<Iterator>::parseInt(Iterator iter)
  {
    unsigned int out = 0;
    for (; *iter >= '0' && *iter <= '9'; ++iter)
      out = out * 10 + (*iter - '0');
    return out;
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handlePosition(Iterator& iter)
  {
    auto end = findIntegerEnd(iter);

    if (iter == end || *end != '$')
    {
      position = POSITION_NONE;
      return;
    }

    position = parseInt(iter);

    iter = end+1;
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handleFlags(Iterator& iter)
  {
    flags = 0;
    while (true)
    {
      switch (*iter)
      {
        case '-': flags |= FLAG_LEFT_JUSTIFY; break;
        case '+': flags |= FLAG_SHOW_SIGN; break;
        case '#': flags |= FLAG_EXPLICIT_BASE; break;
        case '0': flags |= FLAG_FILL_ZERO; break;
        case ' ': flags |= FLAG_ADD_SPACE; break;
        default:
          return;
      }

      ++iter;
    }
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handleWidth(Iterator& iter)
  {
    if (*iter == '*')
    {
      width = WIDTH_ARG;
      ++iter;
      return;
    }

    auto end = findIntegerEnd(iter);

    if (iter == end)
    {
      width = WIDTH_EMPTY;
      return;
    }

    width = parseInt(iter);

    iter = end;
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handlePrecision(Iterator& iter)
  {
    if (*iter != '.')
    {
      precision = WIDTH_EMPTY;
      return;
    }

    ++iter;

    if (*iter == '*')
    {
      precision = WIDTH_ARG;
      ++iter;
      return;
    }

    auto end = findIntegerEnd(iter);
    if (end == iter)
    {
      precision = 0;
      return;
    }

    precision = parseInt(iter);

    iter = end;
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handleFormatChar(Iterator& iter)
  {
    switch (*iter)
    {
      case 's':
      case 'c':
      case 'b':
      case 'd':
      case 'o':
      case 'x':
      case 'X':
      case 'p':
      case 'e':
      case 'E':
      case 'f':
      case 'F':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
      case '(':
        formatChar = *iter;
        break;
      default:
        FORMAT_ERROR(FormatError::InvalidFormatter);
    }

    ++iter;
  }

  template <typename Iterator>
  inline void FormatterItem<Iterator>::handleFormatter(Iterator& iter)
  {
    handlePosition(iter);
    handleFlags(iter);
    handleWidth(iter);
    handlePrecision(iter);
    handleFormatChar(iter);

    fixFlags();
  }
}

template <typename Streambuf>
class Formatter
{
  public:
    typedef typename std::remove_reference<Streambuf>::type streambuf_type;
    typedef typename streambuf_type::char_type char_type;
    typedef typename streambuf_type::traits_type traits_type;

    Formatter(Streambuf& stream);

    template <typename... Args>
    void print(const char_type* format, Args... args);

  private:
    typedef _Formatter::FormatterItem<const char_type*> FormatterItem;

    Streambuf& m_streambuf;

    const char_type* findEndParen(const char_type* begin);

    template <typename... Args>
    void printArgN(const FormatterItem& fmt, Args... args);
    template <typename Arg1, typename... Args>
    void printArgN(unsigned int item, const FormatterItem& fmt,
        Arg1 arg1, Args... args);
    void printArgN(unsigned int item, const FormatterItem& fmt);

    template <typename T>
    void printArg(const FormatterItem& fmt, T arg);

    void printPreFill(
        const FormatterItem& fmt, unsigned int size);
    void printPostFill(
        const FormatterItem& fmt, unsigned int size);
    void printWithEscape(const char_type* begin, const char_type* end);

    void printGeneric(const FormatterItem&, bool arg);
    void printGeneric(const FormatterItem&, char_type arg);
    void printGeneric(const FormatterItem&, const char_type* arg);
    template <typename T>
    typename std::enable_if<
        !std::is_integral<T>::value &&
        !std::is_floating_point<T>::value &&
        !std::is_convertible<T,
          std::basic_string<char_type, traits_type>>::value &&
        !std::is_pointer<T>::value &&
        !_Formatter::is_iterable<T>::value
      >::type printGeneric(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value>::type
      printGeneric(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<std::is_pointer<T>::value>::type
      printGeneric(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value>::type
      printGeneric(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<
        _Formatter::is_iterable<T>::value &&
        !std::is_convertible<T,
          std::basic_string<typename Formatter::char_type,
            typename Formatter::traits_type>>::value
      >::type printGeneric(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<
        !std::is_floating_point<T>::value &&
        !std::is_integral<T>::value &&
        std::is_convertible<T, std::basic_string<typename Formatter::char_type,
          typename Formatter::traits_type>>::value
      >::type printGeneric(const FormatterItem& fmt, T arg);

    template <typename T>
    typename std::enable_if<_Formatter::is_iterable<T>::value>::type
      printContainer(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<!_Formatter::is_iterable<T>::value>::type
      printContainer(const FormatterItem& fmt, T arg);

    template <typename T>
    typename std::enable_if<std::is_convertible<T, char_type>::value>::type
      printChar(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<!std::is_convertible<T, char_type>::value>::type
      printChar(const FormatterItem& fmt, T arg);

    template <typename T>
    typename std::enable_if<std::is_pointer<T>::value>::type
      printPointer(const FormatterItem& fmt, T arg);
    template <typename T>
    typename std::enable_if<!std::is_pointer<T>::value>::type
      printPointer(const FormatterItem& fmt, T arg);

    template <unsigned int base, typename T>
    typename std::enable_if<
        _Formatter::is_integral<T>::value
      >::type printIntegral(const FormatterItem& fmt, T value);
    template <unsigned int Tbase, typename T>
    std::size_t printIntegral(char_type* str,
        const FormatterItem& fmt, T value);
    template <unsigned int base, typename T>
    typename std::enable_if<
        !_Formatter::is_integral<T>::value
      >::type printIntegral(const FormatterItem& fmt, T value);

    template <unsigned int base, typename T>
    typename std::enable_if<
        _Formatter::is_integral<T>::value
      >::type printUnsigned(const FormatterItem& fmt, T value);
    template <unsigned int base, typename T>
    typename std::enable_if<
        !_Formatter::is_integral<T>::value
      >::type printUnsigned(const FormatterItem& fmt, T value);

    template <typename T>
    typename std::enable_if<
        std::is_floating_point<T>::value
      >::type printFloat(const FormatterItem& fmt, T value);
    template <typename T>
    typename std::enable_if<
        !std::is_floating_point<T>::value
      >::type printFloat(const FormatterItem& fmt, T value);
};

template <typename Streambuf>
inline Formatter<Streambuf>::Formatter(Streambuf& stream) :
  m_streambuf(stream)
{
}

template <typename Streambuf>
template <typename... Args>
void Formatter<Streambuf>::print(const char_type* format, Args... args)
{
  bool positional = false;
  unsigned int position = 0;
  const char_type* last = format;
  auto iter = format;
  while (true)
  {
    switch (*iter)
    {
      case '\0':
        m_streambuf.sputn(last, iter-last);
        last = iter;
        return;
      case '%':
        m_streambuf.sputn(last, iter-last);
        ++iter;
        switch (*iter)
        {
          case '%':
            ++iter;
            m_streambuf.sputc('%');
            break;
          default:
            {
              FormatterItem fmt;
              fmt.handleFormatter(iter);

              // if no position is provided, set it ourselves
              if (fmt.position == FormatterItem::POSITION_NONE)
                fmt.position = position;
              // else, switch to positionnal mode
              else
              {
                positional = true;
                position = fmt.position;
              }

              if (!positional)
                ++position;

              if (fmt.formatChar == '(')
              {
                auto endblock = findEndParen(iter);

                fmt.ctnBegin = iter;
                fmt.ctnEnd = endblock-2;

                iter = endblock;
              }

              printArgN(fmt, args...);
            }
            break;
        }
        last = iter;
        break;
      default:
        ++iter;
    }
  }
}

template <typename Streambuf>
const typename Formatter<Streambuf>::char_type*
  Formatter<Streambuf>::findEndParen(const char_type* begin)
{
  // find the end
  unsigned int level = 1;
  auto endblock = begin;
  while (level)
  {
    switch (*endblock)
    {
      case '\0':
        FORMAT_ERROR(FormatError::InvalidFormatter);
        break;
      case '%':
        ++endblock;
        switch (*endblock)
        {
          case '(':
            ++level;
            break;
          case ')':
            --level;
            break;
        }
        ++endblock;
        break;
      default:
        ++endblock;
    }
  }

  return endblock;
}

template <typename Streambuf>
template <typename... Args>
inline
void Formatter<Streambuf>::printArgN(const FormatterItem& fmt,
    Args... args)
{
  printArgN(fmt.position, fmt, args...);
}

template <typename Streambuf>
template <typename Arg1, typename... Args>
inline
void Formatter<Streambuf>::printArgN(unsigned int item,
    const FormatterItem& fmt, Arg1 arg1, Args... args)
{
  if (item)
    return printArgN(item-1, fmt, args...);

  printArg(fmt, arg1);
}

template <typename Streambuf>
template <typename T>
inline
void Formatter<Streambuf>::printArg(
    const FormatterItem& fmt, T arg)
{
  switch (fmt.formatChar)
  {
    case 's':
      printGeneric(fmt, arg);
      break;
    case 'c':
      printChar(fmt, arg);
      break;
    case 'b':
      printUnsigned<2>(fmt, arg);
      break;
    case 'd':
      printIntegral<10>(fmt, arg);
      break;
    case 'o':
      printUnsigned<8>(fmt, arg);
      break;
    case 'x':
    case 'X':
      printUnsigned<16>(fmt, arg);
      break;
    case 'p':
      printPointer(fmt, arg);
      break;
    case 'e':
    case 'E':
    case 'f':
    case 'F':
    case 'g':
    case 'G':
      printFloat(fmt, arg);
      break;
    case 'a':
    case 'A':
      FORMAT_ERROR(FormatError::NotImplemented);
      break;
    case '(':
      printContainer(fmt, arg);
      break;
    default:
      // should not be here
      assert(false);
      break;
  }
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printArgN(unsigned int,
    const FormatterItem&)
{
  FORMAT_ERROR(FormatError::TooFewArguments);
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printPreFill(
    const FormatterItem& fmt, unsigned int size)
{
  if (fmt.width == FormatterItem::WIDTH_EMPTY)
    return;

  size = fmt.width - size;
  if (!(fmt.flags & FormatterItem::FLAG_LEFT_JUSTIFY))
    while (size--)
      m_streambuf.sputc(' ');
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printPostFill(
    const FormatterItem& fmt, unsigned int size)
{
  if (fmt.width == FormatterItem::WIDTH_EMPTY)
    return;

  size = fmt.width - size;
  if (fmt.flags & FormatterItem::FLAG_LEFT_JUSTIFY)
    while (size--)
      m_streambuf.sputc(' ');
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printWithEscape(
    const char_type* iter, const char_type* end)
{
  for (; iter < end; ++iter)
    if (*iter == '%')
    {
      m_streambuf.sputc('%');
      ++iter;
      if (*iter != '%')
        FORMAT_ERROR(FormatError::InvalidFormatter);
    }
    else
      m_streambuf.sputc(*iter);
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printGeneric(
    const FormatterItem& fmt, bool arg)
{
  static const char_type t[] =
    {'t', 'r', 'u', 'e'};
  static const char_type f[] =
    {'f', 'a', 'l', 's', 'e'};

  printPreFill(fmt, arg ? sizeof(t)/sizeof(*t) : sizeof(f)/sizeof(*f));

  if (arg)
    m_streambuf.sputn(t, sizeof(t)/sizeof(*t));
  else
    m_streambuf.sputn(f, sizeof(f)/sizeof(*f));

  printPostFill(fmt, arg ? sizeof(t)/sizeof(*t) : sizeof(f)/sizeof(*f));
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printGeneric(
    const FormatterItem& fmt, char_type arg)
{
  printChar(fmt, arg);
}

template <typename Streambuf>
inline
void Formatter<Streambuf>::printGeneric(
    const FormatterItem& fmt, const char_type* arg)
{
  // calculate size
  std::size_t size;
  {
    const char_type* iter;
    for (iter = arg; *iter; ++iter)
      ;
    size = iter - arg;
  }

  // print
  printPreFill(fmt, size);

  m_streambuf.sputn(arg, size);

  printPostFill(fmt, size);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<
    !std::is_integral<T>::value &&
    !std::is_floating_point<T>::value &&
    !std::is_convertible<T,
      std::basic_string<typename Formatter<Streambuf>::char_type,
        typename Formatter<Streambuf>::traits_type>>::value &&
    !std::is_pointer<T>::value &&
    !_Formatter::is_iterable<T>::value
  >::type Formatter<Streambuf>::printGeneric(
      const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<std::is_integral<T>::value>::type
  Formatter<Streambuf>::printGeneric(
      const FormatterItem& fmt, T arg)
{
  printIntegral<10>(fmt, arg);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<std::is_pointer<T>::value>::type
  Formatter<Streambuf>::printGeneric(
      const FormatterItem& fmt, T arg)
{
  printPointer(fmt, arg);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<std::is_floating_point<T>::value>::type
  Formatter<Streambuf>::printGeneric(
      const FormatterItem& fmt, T arg)
{
  FormatterItem fmt2 = fmt;
  fmt2.formatChar = 'g';
  fmt2.fixFlags();

  printFloat(fmt2, arg);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<
  !std::is_floating_point<T>::value &&
  !std::is_integral<T>::value &&
  std::is_convertible<T,
    std::basic_string<
      typename Formatter<Streambuf>::char_type,
      typename Formatter<Streambuf>::traits_type>>::value
  >::type Formatter<Streambuf>::printGeneric(
      const FormatterItem& fmt, T arg)
{
  std::basic_string<
      typename Formatter<Streambuf>::char_type,
      typename Formatter<Streambuf>::traits_type> str(arg);

  printPreFill(fmt, str.length());

  m_streambuf.sputn(str.c_str(), str.length());

  printPostFill(fmt, str.length());
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<
  _Formatter::is_iterable<T>::value &&
  !std::is_convertible<T,
    std::basic_string<
      typename Formatter<Streambuf>::char_type,
      typename Formatter<Streambuf>::traits_type>>::value
  >::type Formatter<Streambuf>::printGeneric(const FormatterItem& fmt, T arg)
{
  const char_type subfmt[] = "%s, ";

  FormatterItem fmt2 = fmt;
  fmt2.ctnBegin = subfmt;
  fmt2.ctnEnd = subfmt + sizeof(subfmt)-1;

  m_streambuf.sputc('{');

  printContainer(fmt2, arg);

  m_streambuf.sputc('}');
}

template <typename Streambuf>
template <typename T>
typename std::enable_if<_Formatter::is_iterable<T>::value>::type
  Formatter<Streambuf>::printContainer(
      const FormatterItem& fmt, T arg)
{
  const auto begin = fmt.ctnBegin;
  const auto end = fmt.ctnEnd;

  FormatterItem subfmt;
  // if we have an escaped percent before the separator, put this to true
  bool percent = false;
  auto iter = begin;
  auto formatterPos = end;
  auto formatterEnd = end;
  auto separatorPos = end;
  auto separatorEnd = end;
  while (iter < end)
  {
    switch (*iter)
    {
      case '\0':
        // should not be here
        assert(false);
        return;
      case '%':
        ++iter;
        switch (*iter)
        {
          case '%':
            if (separatorPos == end)
              percent = true;
            ++iter;
            break;
          case '|':
            separatorPos = iter-1;
            separatorEnd = iter+1;
            // nothing more to do, exit the loop
            iter = end;
            break;
          default:
            if (formatterPos == end)
            {
              formatterPos = iter-1;

              subfmt.handleFormatter(iter);

              if (subfmt.formatChar == '(')
              {
                auto endblock = findEndParen(iter);

                subfmt.ctnBegin = iter;
                subfmt.ctnEnd = endblock-2;

                iter = endblock;
              }

              formatterEnd = iter;
            }
            break;
        }
        break;
      default:
        ++iter;
    }
  }

  if (formatterPos == end)
    FORMAT_ERROR(FormatError::InvalidFormatter);

  if (separatorPos == end)
    separatorEnd = separatorPos = formatterEnd;

  bool first = true;
  for (const auto& item : arg)
  {
    if (!first)
      m_streambuf.sputn(separatorEnd, end-separatorEnd);
    else
      first = false;

    if (percent)
      printWithEscape(begin, formatterPos);
    else
      m_streambuf.sputn(begin, formatterPos-begin);

    printArg(subfmt, item);

    if (percent)
      printWithEscape(formatterEnd, separatorPos);
    else
      m_streambuf.sputn(formatterEnd, separatorPos-formatterEnd);
  }
}

template <typename Streambuf>
template <typename T>
typename std::enable_if<!_Formatter::is_iterable<T>::value>::type
  Formatter<Streambuf>::printContainer(
      const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<std::is_convertible<T,
    typename Formatter<Streambuf>::char_type>::value>::type
  Formatter<Streambuf>::printChar(const FormatterItem& fmt, T arg)
{
  printPreFill(fmt, 1);

  m_streambuf.sputc(arg);

  printPostFill(fmt, 1);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<!std::is_convertible<T,
    typename Formatter<Streambuf>::char_type>::value>::type
  Formatter<Streambuf>::printChar(const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<std::is_pointer<T>::value>::type
  Formatter<Streambuf>::printPointer(const FormatterItem& fmt, T arg)
{
  FormatterItem fmt2 = fmt;

  fmt2.flags = FormatterItem::FLAG_EXPLICIT_BASE;
  fmt2.precision = sizeof(void*) * 2;
  fmt2.formatChar = 'x';

  printIntegral<16>(fmt2, reinterpret_cast<uintptr_t>(arg));
}

template <typename Streambuf>
template <typename T>
inline
typename std::enable_if<!std::is_pointer<T>::value>::type
  Formatter<Streambuf>::printPointer(const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

namespace _Formatter
{
  template <typename T>
  inline
  typename std::enable_if<std::is_signed<T>::value, bool>::type
    isNegative(T value)
  {
    return value < 0;
  }

  template <typename T>
  inline
  typename std::enable_if<!std::is_signed<T>::value, bool>::type
    isNegative(T)
  {
    return false;
  }
}

template <typename Streambuf>
template <unsigned int Tbase, typename T>
typename std::enable_if<
    _Formatter::is_integral<T>::value
  >::type Formatter<Streambuf>::printIntegral(
      const FormatterItem& fmt, T value)
{
  // convert the number

  char_type buf[64];
  std::size_t numsize =
      printIntegral<Tbase>(buf + sizeof(buf)/sizeof(*buf), fmt, value),
    size = numsize;

  unsigned int zerofill;

  if (fmt.precision == FormatterItem::WIDTH_ARG)
  {
    FORMAT_ERROR(FormatError::NotImplemented);
  }
  else if (fmt.precision == FormatterItem::WIDTH_EMPTY)
    zerofill = 1;
  else
    zerofill = fmt.precision;

  if (zerofill > size)
    zerofill = zerofill - size;
  else
    zerofill = 0;

  // calculate size

  if ((_Formatter::isNegative(value) && fmt.formatChar == 'd') ||
      (fmt.flags & FormatterItem::FLAG_SHOW_SIGN) ||
      (fmt.flags & FormatterItem::FLAG_ADD_SPACE))
    ++size;
  else if (fmt.flags & FormatterItem::FLAG_EXPLICIT_BASE)
    switch (fmt.formatChar)
    {
      case 'x': 
      case 'X':
        if (value != 0)
          size += 2;
        break;
      case 'o': 
        ++size;
        break;
      default:
        // should not be here
        assert(false);
    }

  // get width and calculate needed fill size

  unsigned int fill;
  if (fmt.width == FormatterItem::WIDTH_ARG)
  {
    FORMAT_ERROR(FormatError::NotImplemented);
  }
  else if (fmt.width == FormatterItem::WIDTH_EMPTY)
    fill = 0;
  else
    fill = fmt.width;

  if (static_cast<int>(fill) - static_cast<int>(size) -
        static_cast<int>(zerofill) >= 0)
    fill = fill - size - zerofill;
  else
    fill = 0;

  // fill before sign (with spaces)

  if (!(fmt.flags & FormatterItem::FLAG_FILL_ZERO) &&
      !(fmt.flags & FormatterItem::FLAG_LEFT_JUSTIFY))
    for (unsigned int i = 0; i < fill; ++i)
      m_streambuf.sputc(' ');

  // show sign or base

  if (fmt.flags & FormatterItem::FLAG_EXPLICIT_BASE)
    switch (fmt.formatChar)
    {
      case 'x':
        if (value != 0)
        {
          m_streambuf.sputc('0');
          m_streambuf.sputc('x');
        }
        break;
      case 'X':
        if (value != 0)
        {
          m_streambuf.sputc('0');
          m_streambuf.sputc('X');
        }
        break;
      case 'o':
        m_streambuf.sputc('0');
        break;
      default:
        // should not be here
        assert(false);
    }
  else if (_Formatter::isNegative(value))
    m_streambuf.sputc('-');
  else if (fmt.flags & FormatterItem::FLAG_SHOW_SIGN)
    m_streambuf.sputc('+');
  else if (fmt.flags & FormatterItem::FLAG_ADD_SPACE)
    m_streambuf.sputc(' ');

  // fill after sign (with zeros)
  if (fmt.flags & FormatterItem::FLAG_FILL_ZERO)
    zerofill += fill;

  for (unsigned int i = 0; i < zerofill; ++i)
    m_streambuf.sputc('0');

  // print number

  m_streambuf.sputn(buf+sizeof(buf)-numsize, numsize);

  if (fmt.flags & FormatterItem::FLAG_LEFT_JUSTIFY)
    for (unsigned int i = 0; i < fill; ++i)
      m_streambuf.sputc(' ');
}

template <typename Streambuf>
template <unsigned int Tbase, typename T>
std::size_t Formatter<Streambuf>::printIntegral(char_type* str,
    const FormatterItem& fmt, T value)
{
  static_assert(Tbase == 2 || Tbase == 8 || Tbase == 10 || Tbase == 16,
      "unsupported base");

  // cast base to same type as T to avoid forcing unsigned cast later
  const T base = Tbase;

  char_type baseLetter;
  if (fmt.formatChar == 'X')
    baseLetter = 'A';
  else
    baseLetter = 'a';

  char_type* ptr = str-1;

  while (value)
  {
    typename std::make_signed<char_type>::type digit = value % base;

    value /= base;

    if (digit < 0)
    {
      digit = -digit;
      // we can invert here without losing precision since we /= 10 above
      value = -value;
    }

    if (digit >= 10)
      digit += baseLetter - 10;
    else
      digit += '0';

    *ptr = digit;

    --ptr;
  }

  return str-ptr-1;
}

template <typename Streambuf>
template <unsigned int base, typename T>
inline
typename std::enable_if<
    !_Formatter::is_integral<T>::value
  >::type Formatter<Streambuf>::printIntegral(
      const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf>
template <unsigned int base, typename T>
inline
typename std::enable_if<
    _Formatter::is_integral<T>::value
  >::type Formatter<Streambuf>::printUnsigned(
      const FormatterItem& fmt, T value)
{
  printIntegral<base, typename std::make_unsigned<T>::type>(fmt, value);
}

template <typename Streambuf>
template <unsigned int base, typename T>
inline
typename std::enable_if<
    !_Formatter::is_integral<T>::value
  >::type Formatter<Streambuf>::printUnsigned(
      const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf>
template <typename T>
typename std::enable_if<
    std::is_floating_point<T>::value
  >::type Formatter<Streambuf>::printFloat(
      const FormatterItem& fmt, T value)
{
  std::ostringstream ss;
  ss << std::setprecision(fmt.precision);
  if (fmt.width != FormatterItem::WIDTH_EMPTY)
    ss << std::setw(fmt.width);
  if (fmt.flags & FormatterItem::FLAG_FILL_ZERO)
    ss << std::internal << std::setfill('0');
  if (fmt.flags & FormatterItem::FLAG_LEFT_JUSTIFY)
    ss << std::left;
  switch (fmt.formatChar)
  {
    case 'f':
    case 'F':
      ss << std::fixed;
      break;
    case 'E':
      ss << std::uppercase;
    case 'e':
      ss << std::scientific;
      break;
    case 'G':
      ss << std::uppercase;
    case 'g':
      break;
    default:
      // should not be here
      assert(false);
  }
  ss << value;
  std::string s = ss.str();
  m_streambuf.sputn(s.c_str(), s.length());
}

template <typename Streambuf>
template <typename T>
typename std::enable_if<
    !std::is_floating_point<T>::value
  >::type Formatter<Streambuf>::printFloat(
      const FormatterItem&, T)
{
  FORMAT_ERROR(FormatError::IncompatibleType);
}

template <typename Streambuf, typename... Args>
inline void writef(Streambuf& streambuf,
    const typename Streambuf::char_type* format, Args... args)
{
  Formatter<Streambuf>(streambuf).print(format, args...);
}

template <typename... Args>
inline void writef(const char* format, Args... args)
{
  Formatter<std::streambuf>(*std::cout.rdbuf()).print(format, args...);
}

template <typename... Args>
inline void writef(const wchar_t* format, Args... args)
{
  Formatter<std::wstreambuf>(*std::wcout.rdbuf()).print(format, args...);
}

}

#endif
// vim: ts=2:sw=2:sts=2:expandtab
