#ifndef __PROXY_H__
#define __PROXY_H__
#include "cache.hpp"
#include "networks.hpp"
#include "request.hpp"

#define PROXY_PORT "12345"

class Proxy {
 public:
  void run();
};
#endif
