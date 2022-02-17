#ifndef __REQUEST_H__
#define __REQUEST_H__

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
#endif
