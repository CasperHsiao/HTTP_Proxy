#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "cache.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>

class Request {
 public:
  std::string request;
  std::string method;
  std::string url;
  std::string protocol_version;
  std::string hostname;
  std::string port;
  int content_length;
  std::string start_line;
  std::unordered_map<std::string, std::string> header;
  std::string body;
};

<<<<<<< HEAD
void handle_request(int connection_fd);
=======
void handle_request(int connection_fd, Cache & LRU_cache);
void handle_get_request(int client_fd, int server_fd, Request & request, Cache & LRU_cache);
>>>>>>> Finish GET system structure
#endif
