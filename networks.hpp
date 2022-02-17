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

#include "httpParser.hpp"

#define BACKLOG 10

void * get_in_addr(struct sockaddr * sa);
int get_listener_socket(const char * port);
int listen_for_connections(int listener_fd);
int get_connected_socket(const char * hostname, const char * port);
size_t send_buffer(int target_fd, const char * buf, size_t len, int flags);
void handle_connect_request(int client_fd, int server_fd, Request & request);
#endif
