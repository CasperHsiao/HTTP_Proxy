#include "networks.hpp"

void test_handle_connect_request() {
  int client_fd = 0;
  int server_fd = 1;
  Request req;
  char msg[] = "HTTP/1.1 200 OK\r\n\r\n";
  std::cout << strlen(msg) << std::endl;
}

int main(int argc, char * argv[]) {
  test_handle_connect_request();
  return EXIT_SUCCESS;
}
