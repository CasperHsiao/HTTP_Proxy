#include "request.hpp"
#include "response.hpp"
#include "cache.hpp"
#include "networks.hpp"

#define FILE_LEN 70000

void handle_request(int coonection_fd, Cache & LRU_cache) {
  // Receive header from client
  char request_file[FILE_LEN];
  int status = recv(coonection_fd, &request_file, FILE_LEN, 0);

  if (status < 0) {
    std::cerr << "Error: cannot connect to client's socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Parse the header
  HttpParser client_parser(request_file);

  // create a request object
  Request client_request = client_parser.parseRequest();

  // std::cout << client_request.hostname << std::endl;
  // std::cout << client_request.port << std::endl;

  // build connection with original server
  int server_fd = get_connected_socket(client_request.hostname.c_str(), client_request.port.c_str());

  if (server_fd < 0) {
    std::cerr << "Error: cannot connect to server's socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Seperate function calls according to method
  if (client_request.method == "CONNECT") {
    handle_connect_request(coonection_fd, server_fd, client_request);
  }
  else if (client_request.method == "POST") {
    std::cout << "Post\n";
  }
  else if (client_request.method == "GET") {
    handle_get_request(coonection_fd, server_fd, client_request, LRU_cache);
  }
  // Shit down both client and server's socket connections
  close(coonection_fd);
  close(server_fd);
}

void handle_get_request(int client_fd, int server_fd, Request & request, Cache & LRU_cache){
    std::string client_request_url = request.url;
    std::unordered_map<std::string, std::string>::const_iterator fetch_cache = LRU_cache.cache_data.find(client_request_url);

    //std::cout<< "URL: " << client_request_url<< std::endl;

    if(fetch_cache == LRU_cache.cache_data.end()){
      send(server_fd, request.request.c_str(), request.request.length(), 0);
      handle_get_response(client_fd, server_fd, LRU_cache);
    }
    else{
        std::cout<< "Cache Exist\n";
    }
}

void handle_get_response(int client_fd, int server_fd, Cache & LRU_cache){
  std::cout << "Success send response\n";


  // Receive response from server
  char response_file[FILE_LEN];
  int status = recv(server_fd, &response_file, FILE_LEN, 0);
  if (status < 0) {
    std::cerr << "Error: cannot receive server's response" << std::endl;
    exit(EXIT_FAILURE);
  }
  send(client_fd, response_file, FILE_LEN, 0);
}
