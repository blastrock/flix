#ifndef SERIAL_HPP
#define SERIAL_HPP

#include <cstdint>

class Serial
{
public:
  explicit Serial(uint16_t port)
    : _port(port)
  {}

  void init();
  void write(char c);

private:
  uint16_t _port;
};

Serial* getCom1();

#endif /* SERIAL_HPP */
