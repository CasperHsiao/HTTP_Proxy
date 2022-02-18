#include "networks.hpp"

void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_listener_socket(const char * port) {
  int sock_fd;
  struct addrinfo host_info, *host_info_list, *p;
  const char * hostname = NULL;
  int yes = 1;
  int rv;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;  // use my IP

  if ((rv = getaddrinfo(hostname, port, &host_info, &host_info_list)) != 0) {
    std::cerr << "Error: failed to get address info from the host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    throw std::exception();
  }

  // loop through all the results and bind to the first we can
  for (p = host_info_list; p != NULL; p = p->ai_next) {
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      std::cerr << "Error: failed to create server socket" << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      std::cerr << "Error: failed to set socket option to allow address reuse"
                << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      throw std::exception();
    }

    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      std::cerr << "Error: failed to bind socket to the host" << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    std::cerr << "Error: failed to create a listener socket" << std::endl;
    throw std::exception();
  }

  if (listen(sock_fd, BACKLOG) == -1) {
    std::cerr << "Error: failed to listen on the server socket" << std::endl;
    throw std::exception();
  }
  freeaddrinfo(host_info_list);  // all done with this structure

  return sock_fd;
}

void handle_request(int connection_fd) {
  // Receive header from client
  std::string http_request;
  int nbytes = recv_http_message_header(connection_fd, http_request, 0);

  if (nbytes <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  std::cout << http_request << std::endl;
  // Parse the header
  HttpParser client_parser(http_request);

  // create a request object
  Request client_request = client_parser.parseRequest();

  // receive rest of the body
  nbytes = recv_http_message_body(connection_fd,
                                  client_request.body,
                                  client_request.request,
                                  0,
                                  client_request.content_length);

  if (nbytes <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  // build connection with original server
  int server_fd =
      get_connected_socket(client_request.hostname.c_str(), client_request.port.c_str());

  if (server_fd < 0) {
    std::cerr << "Error: cannot connect to server's socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Seperate function calls according to method
  if (client_request.method == "CONNECT") {
    handle_connect_request(connection_fd, server_fd, client_request);
  }
  else if (client_request.method == "POST") {
    handle_post_request(connection_fd, server_fd, client_request);
  }
  else if (client_request.method == "GET") {
    std::cout << "Get\n";
  }
  // Shut down both client and server's socket connections
  close(connection_fd);
  close(server_fd);
}

void listen_for_connections(int listener_fd) {  // ONLY accept once for now
  int new_fd;
  struct sockaddr_storage client_addr;  // connector's address information
  socklen_t addrlen = sizeof(client_addr);
  char clientIP[INET6_ADDRSTRLEN];

  struct pollfd * pfds = new struct pollfd();
  pfds[0].fd = listener_fd;
  pfds[0].events = POLLIN;
  while (true) {
    int poll_count = poll(pfds, 1, -1);
    if (poll_count == -1) {
      std::cerr << "Error: poll_count accept error" << std::endl;
      throw std::exception();
    }

    new_fd = accept(pfds[0].fd, (struct sockaddr *)&client_addr, &addrlen);
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr),
              clientIP,
              INET6_ADDRSTRLEN);
    if (new_fd == -1) {
      std::cerr << "Error: failed to accpet client connection from " << clientIP
                << std::endl;
      std::cout << "Server: keep listening" << std::endl;
    }
    std::thread(handle_request, new_fd).detach();
  }
  delete pfds;
}

int get_connected_socket(const char * hostname, const char * port) {
  int server_fd;
  struct addrinfo server_info, *server_info_list, *p;
  char serverIP[INET6_ADDRSTRLEN];
  int rv;

  memset(&server_info, 0, sizeof(server_info));
  server_info.ai_family = AF_UNSPEC;
  server_info.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(hostname, port, &server_info, &server_info_list)) != 0) {
    std::cerr << "Error: cannot get address info for server" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  // loop through all the results and connect to the first we can
  for (p = server_info_list; p != NULL; p = p->ai_next) {
    if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      //perror("client: socket");
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      //perror("client: connect");
      std::cerr << "Error: cannot connect to server" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    std::cerr << "Error: failed to connect to ringmaster" << std::endl;
    return -1;
  }

  inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            serverIP,
            sizeof(serverIP));
  freeaddrinfo(server_info_list);  // all done with this structure
  return server_fd;
}

ssize_t send_buffer(int target_fd, const char * buf, size_t len, int flags) {
  ssize_t bytes_sent;
  ssize_t total_bytes_sent = 0;
  size_t bytes_left = len;
  const char * buf_left = buf;
  while (bytes_left > 0) {
    if ((bytes_sent = send(target_fd, buf_left, bytes_left, flags)) == -1) {
      std::cerr << "Error: failed to send entire buffer" << std::endl;
      return -1;
    }
    total_bytes_sent += bytes_sent;
    bytes_left -= bytes_sent;
    buf_left += bytes_sent;
  }
  return total_bytes_sent;
}

ssize_t recv_http_message_with_delimiter(int target_fd,
                                         std::string & message,
                                         std::string & delimiter,
                                         int flags) {
  std::vector<char> buf;
  buf.resize(HTTP_MSG_BUFFER_SIZE, '\0');
  size_t bytes_recv;
  size_t total_bytes_recv = 0;
  while (true) {
    size_t remaining_size = buf.size() - total_bytes_recv;
    if (remaining_size < HTTP_MSG_BUFFER_SIZE) {
      buf.resize(buf.size() * 2, '\0');
      remaining_size = buf.size() - total_bytes_recv;
    }
    if ((bytes_recv = recv(
             target_fd, &buf.data()[total_bytes_recv], remaining_size, flags)) <= 0) {
      return bytes_recv;
    }
    total_bytes_recv += bytes_recv;
    std::string buf_string(buf.begin(), buf.begin() + total_bytes_recv);
    if (buf_string.find(delimiter) != std::string::npos) {  // found delimiter
      message = buf_string;
      return total_bytes_recv;
    }
  }
}

ssize_t recv_http_message_header(int target_fd, std::string & message, int flags) {
  std::string empty_line("\r\n\r\n");
  int nbytes;
  if ((nbytes = recv_http_message_with_delimiter(
           target_fd, message, empty_line, flags)) <= 0) {
    std::cerr << "Error: failed to receive request header" << std::endl;
  }
  return nbytes;
}

ssize_t recv_http_message_body(int target_fd,
                               std::string & body,
                               std::string & full_message,
                               int flags,
                               int content_length) {
  if (content_length == -1) {  // message is chunked
    std::string delimiter("0\r\n");
    if (body.find(delimiter) != std::string::npos) {  // body already received
      return 1;
    }
    std::string rest_of_body;
    int nbytes;
    if ((nbytes = recv_http_message_with_delimiter(
             target_fd, rest_of_body, delimiter, flags)) <= 0) {
      std::cerr << "Error: failed to receive request body" << std::endl;
      return nbytes;
    }
    body += rest_of_body;
    full_message += rest_of_body;
    return nbytes;
  }
  if (content_length == 0) {  // no body to receive
    return 1;
  }
  int bytes_recv;
  int total_bytes_recv = body.size();
  int bytes_left = content_length - total_bytes_recv;
  char * buf = new char[bytes_left];
  char * buf_left = buf;
  while (bytes_left > 0) {
    if ((bytes_recv = recv(target_fd, buf_left, bytes_left, flags)) <= 0) {
      std::cerr << "Error: failed to receive request body" << std::endl;
      return bytes_recv;
    }
    total_bytes_recv += bytes_recv;
    bytes_left -= bytes_recv;
    buf_left += bytes_recv;
  }
  body += buf;
  full_message += buf;
  delete[] buf;
  return total_bytes_recv;
}

void handle_post_request(int client_fd, int server_fd, Request & request) {
  int nbytes;
  if ((nbytes = send_buffer(
           server_fd, request.request.c_str(), request.request.size(), 0)) == -1) {
    std::cerr << "Error: failed to send post request to server" << std::endl;
    return;
  }
}

void handle_connect_request(int client_fd, int server_fd, Request & request) {
  char msg[] = "HTTP/1.1 200 OK\r\n\r\n";
  int nbytes;
  if ((nbytes = send_buffer(client_fd, msg, strlen(msg) + 1, 0)) ==
      -1) {  // +1 accounts null terminator
    std::cerr << "Error: failed to send connect request success to client" << std::endl;
    return;
  }
  struct pollfd * pfds_recv = new struct pollfd[2];
  struct pollfd * pfds_send = new struct pollfd[2];
  // recv() from pfds_recv[i].fd just send() to pfds_send[i].fd
  pfds_recv[0].fd = client_fd;
  pfds_recv[0].events = POLLIN;
  pfds_send[0].fd = server_fd;
  pfds_send[0].events = POLLOUT;
  pfds_recv[1].fd = server_fd;
  pfds_recv[1].events = POLLIN;
  pfds_send[1].fd = client_fd;
  pfds_send[1].events = POLLOUT;
  while (true) {
    int poll_count_recv = poll(pfds_recv, 2, -1);
    if (poll_count_recv == -1) {
      std::cerr << "Error: poll_count_recv" << std::endl;
      return;
    }

    for (int i = 0; i < 2; i++) {
      if (pfds_recv[i].revents & POLLIN) {
        int nbytes;
        char buf[CONNECTION_TUNNEL_BUFFER_SIZE];
        if ((nbytes = recv(pfds_recv[i].fd, buf, sizeof(buf), 0)) <= 0) {
          std::cerr << "Error: failed to receive from connection tunnel" << std::endl;
          return;
        }
        if (send_buffer(pfds_send[i].fd, buf, nbytes, 0) == -1) {
          return;
        }
      }
    }
  }
}
