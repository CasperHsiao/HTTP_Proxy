#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
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
  std::string start_line;
  std::unordered_map<std::string, std::string> header;
  std::string body;
};

void handle_request(int listener_fd, int connection_fd);
#endif
