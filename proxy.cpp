
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

<<<<<<< HEAD
=======
  int connection_fd;
  Cache LRU_cache;
>>>>>>> Finish GET system structure
  try {
    listen_for_connections(listener_fd);
    // pthread
<<<<<<< HEAD
=======
    handle_request(connection_fd, LRU_cache);
>>>>>>> Finish GET system structure
  }
  catch (std::exception & e) {
    std::cout << "Error: client_fd" << std::endl;
    return EXIT_FAILURE;
  }
}
