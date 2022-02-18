
#include "networks.hpp"
#include "request.hpp"
#include "cache.hpp"

using namespace std;

#define PROXY_PORT "12345"

int main(int argc, char * argv[]) {
  int listener_fd;
  try {
    listener_fd = get_listener_socket(PROXY_PORT);
  }
  catch (std::exception & e) {
    std::cout << "Error: listener_fd" << std::endl;
    return EXIT_FAILURE;
  }

  int connection_fd;
  Cache LRU_cache;
  try {
    listen_for_connections(listener_fd);
    // pthread
    handle_request(connection_fd, LRU_cache);
  }
  catch (std::exception & e) {
    std::cout << "Error: client_fd" << std::endl;
    return EXIT_FAILURE;
  }
}
