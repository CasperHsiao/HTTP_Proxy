#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <functional>
#include <string>
#include <unordered_map>
class Response {
 public:
  std::string response;
  std::string protocol_version;
  std::string status_code;
  std::string status_text;
  int content_length;
  std::string start_line;
  std::unordered_map<std::string, std::string> header;
  std::string body;
};

void handle_get_response(int client_fd, int server_fd, Request & request, Cache & LRU_cache);
#endif
