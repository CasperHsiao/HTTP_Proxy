#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <vector>

#include "httpParser.hpp"
#define BUFFER_SIZE 32

#define BACKLOG 10

int get_listener_socket(const char * port);
int listen_for_connections(int listener_fd);
int get_connected_socket(const char * hostname, const char * port);
ssize_t send_buffer(int target_fd, const char * buf, size_t len, int flags);
ssize_t recv_http_message_header(int target_fd, std::string & message);
void handle_connect_request(int client_fd, int server_fd, Request & request);
#endif
