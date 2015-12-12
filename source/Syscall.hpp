#ifndef SYSCALL_HPP
#define SYSCALL_HPP

#include <functional>
#include "Interrupt.hpp"

using SyscallReturnType = uint64_t;
using SyscallHandler =
  std::function<SyscallReturnType(const InterruptState& state)>;

namespace sys
{

enum ScId
{
#include "syscalls/syscall_enum.hxx"

  last_id,
};

namespace detail
{

// this class handles void return types
template <typename R, typename... Args>
struct HandlerCaller
{
  static SyscallReturnType call(
      const std::function<R(const InterruptState&, Args...)>& func, const InterruptState& st, Args... args)
  {
    return (SyscallReturnType)func(st, args...);
  }
};

template <typename... Args>
struct HandlerCaller<void, Args...>
{
  static SyscallReturnType call(
      const std::function<void(const InterruptState&, Args...)>& func, const InterruptState& st, Args... args)
  {
    func(st, args...);
    return 0;
  }
};

template <typename R, typename... Args>
struct SysCallExecutor;

template <typename R>
struct SysCallExecutor<R>
{
  std::function<R(const InterruptState&)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R>::call(handler, st);
  }
};

template <typename R, typename A1>
struct SysCallExecutor<R, A1>
{
  std::function<R(const InterruptState&, A1)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R, A1>::call(handler, st, (A1)st.rdi);
  }
};

template <typename R, typename A1, typename A2>
struct SysCallExecutor<R, A1, A2>
{
  std::function<R(const InterruptState&, A1, A2)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R, A1, A2>::call(handler, st, (A1)st.rdi, (A2)st.rsi);
  }
};

template <typename R, typename A1, typename A2, typename A3>
struct SysCallExecutor<R, A1, A2, A3>
{
  std::function<R(const InterruptState&, A1, A2, A3)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R, A1, A2, A3>::call(
        handler, st, (A1)st.rdi, (A2)st.rsi, (A3)st.rdx);
  }
};

template <typename R, typename A1, typename A2, typename A3, typename A4>
struct SysCallExecutor<R, A1, A2, A3, A4>
{
  std::function<R(const InterruptState&, A1, A2, A3, A4)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R, A1, A2, A3, A4>::call(
        handler, st, (A1)st.rdi, (A2)st.rsi, (A3)st.rdx, (A4)st.rcx);
  }
};

template <typename R,
    typename A1,
    typename A2,
    typename A3,
    typename A4,
    typename A5>
struct SysCallExecutor<R, A1, A2, A3, A4, A5>
{
  std::function<R(const InterruptState&, A1, A2, A3, A4, A5)> handler;

  SyscallReturnType operator()(const InterruptState& st)
  {
    return HandlerCaller<R, A1, A2, A3, A4, A5>::call(handler,
        st,
        (A1)st.rdi,
        (A2)st.rsi,
        (A3)st.rdx,
        (A4)st.rcx,
        (A5)st.r8);
  }
};

void registerHandler(ScId id, SyscallHandler handler);

template <typename R, typename... T>
struct RegisterHandler
{
  template <typename H>
  static void regist(ScId id, H&& handler)
  {
    registerHandler(id,
        detail::SysCallExecutor<R, T...>{
          [handler = std::forward<H>(handler)](const InterruptState&, T... args){
            return handler(args...);
          }
        });
  }
};

template <typename R, typename... T>
struct RegisterHandler<R, const InterruptState&, T...>
{
  template <typename H>
  static void regist(ScId id, H&& handler)
  {
    registerHandler(id,
        detail::SysCallExecutor<R, T...>{
          [handler = std::forward<H>(handler)](const InterruptState& st, T... args){
            return handler(st, args...);
          }
        });
  }
};

}

template <typename R, typename... T>
void registerHandler(ScId id, std::function<R(T...)> handler)
{
  detail::RegisterHandler<R, T...>::regist(id, std::move(handler));
}

template <typename R, typename... T>
void registerHandler(ScId id, R(*handler)(T...))
{
  detail::RegisterHandler<R, T...>::regist(id, handler);
}

void initSysCalls();

inline SyscallReturnType call(ScId sc)
{
  SyscallReturnType out;
  asm volatile (
      "int $0x80"
      :"=a"(out)
      :"a"(static_cast<uint64_t>(sc)));
  return out;
}

inline SyscallReturnType call(ScId sc, int fd, const void* buf, long size)
{
  SyscallReturnType out;
  asm volatile (
      "int $0x80"
      :"=a"(out)
      :"a"(static_cast<uint64_t>(sc)),
      "D"((uint64_t)fd),
      "S"((uint64_t)buf),
      "d"((uint64_t)size));
  return out;
}

SyscallReturnType handle(const InterruptState& st);

}

#endif
