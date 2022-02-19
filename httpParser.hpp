#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

#include "request.hpp"
#include "response.hpp"
class HttpParser {
 public:
  const std::string message;
  size_t parser_index;

 public:
  HttpParser(const std::string & msg) : message(msg), parser_index(0) {}
  HttpParser(const char * msg) : message(msg), parser_index(0) {}
  Response parseResponse();
  Request parseRequest();

 private:
  void parseStartLine(Request & req);
  void parseStatusLine(Response & resp);
  std::unordered_map<std::string, std::string> parseHeader();
  void parseRequestHostnameAndPort(Request & req);
  int parseContentLength(std::unordered_map<std::string, std::string> header);
  std::string parseBody();

<<<<<<< HEAD
  void parseFields(Response & response);
=======
  void parseFields();
>>>>>>> 967bcffcb4add9e7b40b2383811f0f86868b433d
};

//int getContentLength(std::string message);
void toUpperCase(std::string & data);
#endif
