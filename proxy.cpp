#include "proxy.hpp"

void Proxy::run() {
  int listener_fd;
  try {
    listener_fd = get_listener_socket(PROXY_PORT);
  }
  catch (std::exception & e) {
    std::cout << "Error: listener_fd" << std::endl;
    return;
  }

  Cache * LRU_cache = new Cache();
  try {
    listen_for_connections(listener_fd, *LRU_cache);
  }
  catch (std::exception & e) {
    std::cout << "Error: client_fd" << std::endl;
    return;
  }
}
