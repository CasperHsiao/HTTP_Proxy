
#include "networks.hpp"
#include "request.hpp"

using namespace std;

#define PROXY_PORT "12345"

int main(int argc, char * argv[]) {
  //const char * proxy_port = "12345";

  // if (argc != 2) {
  //   std::cerr << "Usage: proxy <port_num>" << std::endl;
  //   return EXIT_FAILURE;
  // }
  int listener_fd;
  try {
    listener_fd = get_listener_socket(PROXY_PORT);
  }
  catch (std::exception & e) {
    std::cout << "Error: listener_fd" << std::endl;
    return EXIT_FAILURE;
  }

  try {
    listen_for_connections(listener_fd);
    // pthread
  }
  catch (std::exception & e) {
    std::cout << "Error: client_fd" << std::endl;
    return EXIT_FAILURE;
  }
}
