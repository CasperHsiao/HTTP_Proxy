#include "networks.hpp"

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

void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
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
    //return new_fd;
  }
  delete pfds;
}

void handle_connect_request(int client_fd, int server_fd, Request & request) {
  char msg[] = "HTTP/1.1 200 OK\r\n\r\n";
  int nbytes;
  if ((nbytes = send(client_fd, msg, strlen(msg) + 1, 0)) ==
      -1) {  // +1 accounts null terminator
    std::cerr << "Error: failed to send connect request success to client" << std::endl;
    throw std::exception();
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
      throw std::exception();
    }

    for (int i = 0; i < 2; i++) {
      if (pfds_recv[i].revents & POLLIN) {
        int nbytes;
        char buf[65536];
        if ((nbytes = recv(pfds_recv[i].fd, buf, sizeof(buf), 0)) <= 0) {
          std::cerr << "Error: failed to receive from connection tunnel" << std::endl;
          throw std::exception();
        }
        while (true) {
          int poll_count_send = poll(&pfds_send[i], 1, -1);
          if (poll_count_send == -1) {
            std::cerr << "Error: poll_count_send" << std::endl;
            throw std::exception();
          }
          if ((nbytes = send(pfds_send[i].fd, buf, nbytes, 0)) == -1) {
            std::cerr << "Error: failed to send to connection tunnel" << std::endl;
            throw std::exception();
          }
          break;
        }
      }
    }
  }
}
