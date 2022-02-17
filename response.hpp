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
  std::string start_line;
  std::unordered_map<std::string, std::string> header;
  std::string body;
};
#endif
