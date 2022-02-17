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

#define BACKLOG 10
//#define LISTENING_PORT "8000"

void * get_in_addr(struct sockaddr * sa);
int get_listener_socket(const char * port);
void listen_for_requests(int listener_fd);

#endif
