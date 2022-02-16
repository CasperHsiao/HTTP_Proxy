#include "networks.hpp"

int main(int argc, char * argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: proxy <port_num>" << std::endl;
    return EXIT_FAILURE;
  }
  int listener_fd;
  try {
    listener_fd = get_listener_socket(argv[1]);
  }
  catch (std::exception & e) {
    std::cout << "Error: listener_fd" << std::endl;
    return EXIT_FAILURE;
  }

  try {
    listen_for_requests(listener_fd);
  }
  catch (std::exception & e) {
    std::cout << "Error: client_fd" << std::endl;
    return EXIT_FAILURE;
  }
}
