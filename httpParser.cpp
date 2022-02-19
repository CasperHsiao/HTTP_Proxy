#include "httpParser.hpp"

void toUpperCase(std::string & data) {
  std::for_each(data.begin(), data.end(), [](char & c) { c = ::toupper(c); });
}

// ALMOST the same code as parseStartLine method - VIOLATE DRY
void HttpParser::parseStatusLine(Response & resp) {
  std::string delimiter(" ");
  size_t index = message.find(delimiter, parser_index);
  resp.protocol_version = message.substr(parser_index, index - parser_index);
  parser_index = index + delimiter.size();

  index = message.find(delimiter, parser_index);
  resp.status_code = message.substr(parser_index, index - parser_index);
  parser_index = index + delimiter.size();

  std::string CRLF("\r\n");
  index = message.find(CRLF, parser_index);
  resp.status_text = message.substr(parser_index, index - parser_index);
  resp.start_line = message.substr(0, index);
  parser_index = index + CRLF.size();
}

void HttpParser::parseStartLine(Request & req) {
  std::string delimiter(" ");
  size_t index = message.find(delimiter, parser_index);
  req.method = message.substr(parser_index, index - parser_index);
  parser_index = index + delimiter.size();

  // DANGER: url might have ":<port>"
  index = message.find(delimiter, parser_index);
  req.url = message.substr(parser_index, index - parser_index);
  parser_index = index + delimiter.size();

  std::string CRLF("\r\n");
  index = message.find(CRLF, parser_index);
  req.protocol_version = message.substr(parser_index, index - parser_index);
  req.start_line = message.substr(0, index);
  parser_index = index + CRLF.size();
}

std::unordered_map<std::string, std::string> HttpParser::parseHeader() {
  std::unordered_map<std::string, std::string> header;
  std::string delimiter(": ");
  std::string CRLF("\r\n");
  size_t index = message.find(delimiter, parser_index);
  while (index != std::string::npos) {
    std::string key = message.substr(parser_index, index - parser_index);
    toUpperCase(key);
    parser_index = index + delimiter.size();
    index = message.find(CRLF, parser_index);
    std::string val = message.substr(parser_index, index - parser_index);
    header[key] = val;
    parser_index = index + CRLF.size();
    if (message.find(CRLF, parser_index) ==
        parser_index) {  // reached empty line - \r\n\r\n
      parser_index = parser_index + CRLF.size();
      break;
    }
    index = message.find(delimiter, parser_index);
  }
  return header;
}

void HttpParser::parseRequestHostnameAndPort(Request & req) {
  std::string host_line = req.header["HOST"];
  size_t index;
  std::string delimiter(":");
  if ((index = host_line.find(":")) != std::string::npos) {
    req.hostname = host_line.substr(0, index);
    index = index + delimiter.size();
    req.port = host_line.substr(index);
  }
  else {
    req.hostname = host_line;
    req.port = "80";
  }
}

int HttpParser::parseContentLength(std::unordered_map<std::string, std::string> header) {
  std::unordered_map<std::string, std::string>::const_iterator it =
      header.find("CONTENT-LENGTH");
  if (it != header.end()) {
    std::string length = it->second;
    return stoi(length);
  }
  it = header.find("TRANSFER-ENCODING");
  if (it != header.end()) {
    std::string encoding(it->second);
    toUpperCase(encoding);
    if (encoding == "CHUNKED") {
      return -1;
    }
  }
  return 0;
}

/*
int getContentLength(std::string message) {
  toUpperCase(message);
  std::string targ1("CONTENT-LENGTH: ");
  std::string CRLF("\r\n");
  size_t index = message.find(targ1);
  if (index != std::string::npos) {
    index = index + targ1.size();
    size_t end_index = message.find(CRLF, index);
    std::string length = message.substr(index, end_index - index);
    return stoi(length);
  }
  std::string targ2("TRANSFER-ENCODING: CHUNKED");
  if (message.find(targ2) != std::string::npos) {
    return -1;
  }
  return 0;
}
*/

std::string HttpParser::parseBody() {
  return message.substr(parser_index);
}

Response HttpParser::parseResponse() {
  Response response;
  response.response = message;
  parseStatusLine(response);
  response.header = parseHeader();
  response.content_length = parseContentLength(response.header);
  response.body = parseBody();
  parser_index = 0;
<<<<<<< HEAD
  //parseFields(response);
=======
  parseFields();
>>>>>>> 967bcffcb4add9e7b40b2383811f0f86868b433d
  return response;
}

Request HttpParser::parseRequest() {
  Request request;
  request.request = message;
  parseStartLine(request);
  request.header = parseHeader();
  parseRequestHostnameAndPort(request);
  request.content_length = parseContentLength(request.header);
  request.body = parseBody();
  parser_index = 0;
  return request;
}

<<<<<<< HEAD
// void HttpParser::parseFields(Response & response) {
  
// }
=======
void HttpParser::parseFields() {
  
}
>>>>>>> 967bcffcb4add9e7b40b2383811f0f86868b433d
