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
#include <thread>
#include <vector>

#include "cache.hpp"
#include "httpParser.hpp"
#define HTTP_MSG_BUFFER_SIZE 32
#define CONNECTION_TUNNEL_BUFFER_SIZE 1024

#define BACKLOG 10

void * get_in_addr(struct sockaddr * sa);
ssize_t send_buffer(int target_fd, const char * buf, size_t len, int flags);
ssize_t recv_http_message_header(int target_fd, std::string & message, int flags);
ssize_t recv_http_message_body(int target_fd,
                               std::string & body,
                               std::string & full_message,
                               int flags,
                               int content_length);
int get_listener_socket(const char * port);
void listen_for_connections(int listener_fd, Cache & LRU_cache);
int get_connected_socket(const char * hostname, const char * port);
void handle_request(int connection_fd);
void handle_connect_request(int client_fd, int server_fd, Request & request);
void handle_get_request(int client_fd, int server_fd, Request & request, Cache & LRU_cache);
#endif
