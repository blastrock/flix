#ifndef SYSCALL_HPP
#define SYSCALL_HPP

#include <functional>
#include "Interrupt.hpp"

namespace sys
{

enum ScId
{
  exit = 1,
  print,

  last_id,
};

namespace detail
{

template <typename... Args>
struct SysCallExecutor;

template <>
struct SysCallExecutor<>
{
  std::function<void()> handler;

  void operator()(const InterruptState&)
  {
    handler();
  }
};

template <typename A1>
struct SysCallExecutor<A1>
{
  std::function<void(A1)> handler;

  void operator()(const InterruptState& st)
  {
    handler(reinterpret_cast<A1>(st.rdi));
  }
};

template <typename A1, typename A2>
struct SysCallExecutor<A1, A2>
{
  std::function<void(A1, A2)> handler;

  void operator()(const InterruptState& st)
  {
    handler(reinterpret_cast<A1>(st.rdi), reinterpret_cast<A2>(st.rsi));
  }
};

template <typename A1, typename A2, typename A3>
struct SysCallExecutor<A1, A2, A3>
{
  std::function<void(A1, A2, A3)> handler;

  void operator()(const InterruptState& st)
  {
    handler(reinterpret_cast<A1>(st.rdi), reinterpret_cast<A1>(st.rsi),
        reinterpret_cast<A1>(st.rdx));
  }
};

void registerHandler(ScId id,
    std::function<void(const InterruptState&)> handler);

}

template <typename... T>
void registerHandler(ScId id, std::function<void(T...)> handler)
{
  detail::registerHandler(id,
      detail::SysCallExecutor<T...>{ std::move(handler) });
}

template <typename... T>
void registerHandler(ScId id, void(*handler)(T...))
{
  registerHandler(id, std::function<void(T...)>(handler));
}

void initSysCalls();

inline void call(ScId sc)
{
  asm volatile (
      "int $0x80"
      :
      :"a"(static_cast<uint64_t>(sc)));
}

void handle(const InterruptState& st);

}

#endif
