#ifndef SOCKSPROXY_UTIL_H
#define SOCKSPROXY_UTIL_H
#include <stdio.h>
#include <uv.h>

namespace ws {
namespace util {

struct Addr {
  char ip[16];
  int port;
};

static int readPort(const char *buf, ssize_t offset) {
  return ((unsigned char)buf[0 + offset] << 8) + (unsigned char)buf[1 + offset];
}

static const Addr parseAddr(const char *buf, ssize_t offset) {
  Addr addr;
  addr.port = readPort(buf, offset);
  sprintf(addr.ip, "%d.%d.%d.%d",
          (unsigned char)buf[2+offset],
          (unsigned char)buf[3+offset],
          (unsigned char)buf[4+offset],
          (unsigned char)buf[5+offset]
  );
  return addr;
}

class NoCopyable {
public:
  NoCopyable() = default;
  NoCopyable(const NoCopyable&) = delete;
  NoCopyable&operator=(const NoCopyable&) = delete;
  ~NoCopyable() = default;
};

};

};
#endif //SOCKSPROXY_UTIL_H