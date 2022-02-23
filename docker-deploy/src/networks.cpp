#include "networks.hpp"

// Log file
std::ofstream write_log("/var/log/erss/proxy.log", std::ofstream::out);
std::mutex mtx;

void * get_in_addr(struct sockaddr * sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_listener_socket(const char * port) {
  int sock_fd;
  struct addrinfo host_info, *host_info_list, *p;
  const char * hostname = NULL;
  int yes = 1;
  int rv;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;  // use my IP

  if ((rv = getaddrinfo(hostname, port, &host_info, &host_info_list)) != 0) {
    std::cerr << "Error: failed to get address info from the host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    throw std::exception();
  }

  // loop through all the results and bind to the first we can
  for (p = host_info_list; p != NULL; p = p->ai_next) {
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      std::cerr << "Error: failed to create server socket" << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      std::cerr << "Error: failed to set socket option to allow address reuse"
                << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      throw std::exception();
    }

    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      std::cerr << "Error: failed to bind socket to the host" << std::endl;
      //std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    std::cerr << "Error: failed to create a listener socket" << std::endl;
    throw std::exception();
  }

  if (listen(sock_fd, BACKLOG) == -1) {
    std::cerr << "Error: failed to listen on the server socket" << std::endl;
    throw std::exception();
  }
  freeaddrinfo(host_info_list);  // all done with this structure

  return sock_fd;
}
void handle_request(int connection_fd,
                    int request_id,
                    std::string clientID,
                    Cache & LRU_cache) {
  // output_log({"ID: ", std::to_string(request_id)});

  // Receive header from client
  std::string http_request;
  int nbytes;

  if ((nbytes = recv_http_message_header(connection_fd, http_request, 0)) <= 0) {
    output_log({std::to_string(request_id), ": WARNING can't receive http header"});
    return;  // DANGER: Needs to check error handling
  }
  // Parse the header
  HttpParser client_parser(http_request);

  // create a request object
  Request client_request = client_parser.parseRequest();

  // receive rest of the body
  nbytes = recv_http_message_body(connection_fd,
                                  client_request.body,
                                  client_request.request,
                                  0,
                                  client_request.content_length);

  if (nbytes <= 0) {
    output_log({std::to_string(request_id), ": WARNING can't receive http body"});
    return;  // DANGER: Needs to check error handling
  }

  // build connection with original server
  int server_fd =
      get_connected_socket(client_request.hostname.c_str(), client_request.port.c_str());

  if (server_fd < 0) {
    //std::cerr << "Error: cannot connect to server's socket" << std::endl;
    output_log({std::to_string(request_id), ": ERROR cannot connect to server's socket"});
    return;
  }

  time_t now = time(NULL);
  output_log({std::to_string(request_id),
              ": \"",
              client_request.start_line,
              "\" from ",
              clientID,
              " @ ",
              asctime(gmtime(&now))});
  // Seperate function calls according to method
  if (client_request.method == "CONNECT") {
    output_log({std::to_string(request_id),
                ": Requesting \"",
                client_request.start_line,
                "\" from ",
                client_request.hostname});
    handle_connect_request(connection_fd, server_fd, request_id, client_request);
    output_log({std::to_string(request_id), ": Tunnel closed"});
  }
  else if (client_request.method == "POST") {
    output_log({std::to_string(request_id),
                ": Requesting \"",
                client_request.start_line,
                "\" from ",
                client_request.hostname});
    handle_post_request(connection_fd, server_fd, request_id, client_request);
  }
  else if (client_request.method == "GET") {
    output_log({std::to_string(request_id),
                ": Requesting \"",
                client_request.start_line,
                "\" from ",
                client_request.hostname});
    handle_get_request(connection_fd, server_fd, request_id, client_request, LRU_cache);
  }
  else {
    output_log({std::to_string(request_id), ": Responding HTTP/1.1 400 Bad Request"});
  }
  // Shut down both client and server's socket connections
  close(connection_fd);
  close(server_fd);
}

void listen_for_connections(int listener_fd,
                            Cache & LRU_cache) {  // ONLY accept once for now
  int new_fd;
  int request_uid = 0;
  struct sockaddr_storage client_addr;  // connector's address information
  socklen_t addrlen = sizeof(client_addr);
  char clientIP[INET6_ADDRSTRLEN];

  struct pollfd * pfds = new struct pollfd();
  pfds[0].fd = listener_fd;
  pfds[0].events = POLLIN;
  while (true) {
    int poll_count = poll(pfds, 1, -1);
    if (poll_count == -1) {
      //std::cerr << "Error: poll_count accept error" << std::endl;
      output_log({"(no-id): Error poll_count accept error"});
      continue;
      //throw std::exception();
    }

    new_fd = accept(pfds[0].fd, (struct sockaddr *)&client_addr, &addrlen);
    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr),
              clientIP,
              INET6_ADDRSTRLEN);
    if (new_fd == -1) {
      // std::cerr << "Error: failed to accpet client connection from " << clientIP
      //           << std::endl;
      // std::cout << "Server: keep listening" << std::endl;
      output_log({"(no-id): failed to accpet client connection from ", clientIP});
    }
    std::thread(
        handle_request, new_fd, request_uid++, std::string(clientIP), std::ref(LRU_cache))
        .detach();
  }
  delete pfds;
}

int get_connected_socket(const char * hostname, const char * port) {
  int server_fd;
  struct addrinfo server_info, *server_info_list, *p;
  char serverIP[INET6_ADDRSTRLEN];
  int rv;

  memset(&server_info, 0, sizeof(server_info));
  server_info.ai_family = AF_UNSPEC;
  server_info.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(hostname, port, &server_info, &server_info_list)) != 0) {
    std::cerr << "Error: cannot get address info for server" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  }

  // loop through all the results and connect to the first we can
  for (p = server_info_list; p != NULL; p = p->ai_next) {
    if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      //perror("client: socket");
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }

    if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      //perror("client: connect");
      std::cerr << "Error: cannot connect to server" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      continue;
    }
    break;
  }

  if (p == NULL) {
    std::cerr << "Error: failed to connect to ringmaster" << std::endl;
    return -1;
  }

  inet_ntop(p->ai_family,
            get_in_addr((struct sockaddr *)p->ai_addr),
            serverIP,
            sizeof(serverIP));
  freeaddrinfo(server_info_list);  // all done with this structure
  return server_fd;
}

ssize_t send_buffer(int target_fd, const char * buf, size_t len, int flags) {
  ssize_t bytes_sent;
  ssize_t total_bytes_sent = 0;
  size_t bytes_left = len;
  const char * buf_left = buf;
  while (bytes_left > 0) {
    if ((bytes_sent = send(target_fd, buf_left, bytes_left, MSG_NOSIGNAL)) == -1) {
      std::cerr << "Error: failed to send entire buffer" << std::endl;
      return -1;
    }
    total_bytes_sent += bytes_sent;
    bytes_left -= bytes_sent;
    buf_left += bytes_sent;
  }
  return total_bytes_sent;
}

ssize_t recv_http_message_with_delimiter(int target_fd,
                                         std::string & message,
                                         std::string & delimiter,
                                         int flags) {
  std::vector<char> buf;
  buf.resize(HTTP_MSG_BUFFER_SIZE, '\0');
  size_t bytes_recv;
  size_t total_bytes_recv = 0;
  while (true) {
    size_t remaining_size = buf.size() - total_bytes_recv;
    if (remaining_size < HTTP_MSG_BUFFER_SIZE) {
      buf.resize(buf.size() * 2, '\0');
      remaining_size = buf.size() - total_bytes_recv;
    }
    if ((bytes_recv = recv(
             target_fd, &buf.data()[total_bytes_recv], remaining_size, flags)) <= 0) {
      return bytes_recv;
    }
    total_bytes_recv += bytes_recv;
    std::string buf_string(buf.begin(), buf.begin() + total_bytes_recv);
    if (buf_string.find(delimiter) != std::string::npos) {  // found delimiter
      message = buf_string;
      return total_bytes_recv;
    }
  }
}

ssize_t recv_http_message_header(int target_fd, std::string & message, int flags) {
  std::string empty_line("\r\n\r\n");
  int nbytes;
  if ((nbytes = recv_http_message_with_delimiter(target_fd, message, empty_line, flags)) <
      0) {
    std::cerr << "Error: failed to receive http message header" << std::endl;
  }
  return nbytes;
}

ssize_t recv_http_message_chunked(int target_fd,
                                  std::string & body,
                                  std::string & full_message,
                                  int flags) {
  std::string delimiter("\r\n0\r\n");
  if (body.find(delimiter) != std::string::npos) {  // body already received
    return 1;
  }
  else if (body.find("0\r\n") == 0) {
    return 1;
  }
  std::string rest_of_body;
  int nbytes;
  if ((nbytes = recv_http_message_with_delimiter(
           target_fd, rest_of_body, delimiter, flags)) <= 0) {
    if (nbytes < 0) {
      std::cerr << "Error: failed to receive http message body" << std::endl;
    }
    return nbytes;
  }
  body += rest_of_body;
  full_message += rest_of_body;
  return nbytes;
}

ssize_t recv_http_message_body(int target_fd,
                               std::string & body,
                               std::string & full_message,
                               int flags,
                               int content_length) {
  if (content_length == -1) {  // message is chunked
    return recv_http_message_chunked(target_fd, body, full_message, flags);
  }
  if (content_length == 0) {  // no body to receive
    return 1;
  }
  int bytes_recv;
  int total_bytes_recv = body.size();
  int bytes_left = content_length - total_bytes_recv;
  char * buf = new char[bytes_left + 1];
  buf[bytes_left] = '\0';
  char * buf_left = buf;
  while (bytes_left > 0) {
    if ((bytes_recv = recv(target_fd, buf_left, bytes_left, flags)) <= 0) {
      if (bytes_recv < 0) {
        std::cerr << "Error: failed to receive http message body" << std::endl;
      }
      return bytes_recv;
    }
    total_bytes_recv += bytes_recv;
    bytes_left -= bytes_recv;
    buf_left += bytes_recv;
  }
  body += buf;
  full_message += buf;
  delete[] buf;
  return total_bytes_recv;
}

void handle_post_request(int client_fd,
                         int server_fd,
                         int request_id,
                         Request & request) {
  int nbytes;
  if ((nbytes = send_buffer(
           server_fd, request.request.c_str(), request.request.size(), 0)) == -1) {
    //std::cerr << "Error: failed to send post request to server" << std::endl;
    output_log(
        {std::to_string(request_id), ": ERROR failed to send post request to server"});
    return;
  }
  std::string http_response;
  if ((nbytes = recv_http_message_header(server_fd, http_response, 0)) <= 0) {
    output_log({std::to_string(request_id), ": WARNING can't receive http header"});
    return;
  }
  HttpParser server_parser(http_response);
  Response server_response = server_parser.parseResponse();
  nbytes = recv_http_message_body(server_fd,
                                  server_response.body,
                                  server_response.response,
                                  0,
                                  server_response.content_length);
  if (nbytes <= 0) {
    output_log({std::to_string(request_id), ": WARNING can't receive http body"});
    return;
  }

  if ((nbytes = send_buffer(client_fd,
                            server_response.response.c_str(),
                            server_response.response.size(),
                            0)) == -1) {
    return;
  }
  return;
}

void handle_connect_request(int client_fd,
                            int server_fd,
                            int request_id,
                            Request & request) {
  char msg[] = "HTTP/1.1 200 OK\r\n\r\n";

  // write log
  output_log({std::to_string(request_id), ": Responding \"HTTP/1.1 200 OK\""});

  int nbytes;
  if ((nbytes = send_buffer(client_fd, msg, strlen(msg) + 1, 0)) ==
      -1) {  // +1 accounts null terminator
    std::cerr << "Error: failed to send connect request success to client" << std::endl;
    output_log({std::to_string(request_id),
                ": ERROR failed to send connect request success to client"});
    return;
  }
  struct pollfd * pfds_recv = new struct pollfd[2];
  struct pollfd * pfds_send = new struct pollfd[2];
  // recv() from pfds_recv[i].fd just send() to pfds_send[i].fd
  pfds_recv[0].fd = client_fd;
  pfds_recv[0].events = POLLIN;
  pfds_send[0].fd = server_fd;
  pfds_send[0].events = POLLOUT;
  pfds_recv[1].fd = server_fd;
  pfds_recv[1].events = POLLIN;
  pfds_send[1].fd = client_fd;
  pfds_send[1].events = POLLOUT;
  while (true) {
    int poll_count_recv = poll(pfds_recv, 2, -1);
    if (poll_count_recv == -1) {
      //std::cerr << "Error: poll_count_recv" << std::endl;
      output_log({std::to_string(request_id), ": ERROR poll_count_recv"});
      return;
    }

    for (int i = 0; i < 2; i++) {
      if (pfds_recv[i].revents & POLLIN) {
        int nbytes;
        char buf[CONNECTION_TUNNEL_BUFFER_SIZE];
        if ((nbytes = recv(pfds_recv[i].fd, buf, sizeof(buf), 0)) <= 0) {
          //std::cerr << "Error: failed to receive from connection tunnel" << std::endl;
          if (nbytes < 0) {
            output_log({std::to_string(request_id),
                        ": ERROR failed to receive from connection tunnel"});
          }
          delete[] pfds_recv;
          delete[] pfds_send;
          return;
        }
        if (send_buffer(pfds_send[i].fd, buf, nbytes, MSG_NOSIGNAL) == -1) {
          delete[] pfds_recv;
          delete[] pfds_send;
          return;
        }
      }
    }
  }
}

void handle_get_request(int client_fd,
                        int server_fd,
                        int request_id,
                        Request & request,
                        Cache & LRU_cache) {
  std::string client_request_url = request.url;
  std::unordered_map<std::string, Response>::iterator fetch_cache =
      LRU_cache.cache_data.find(client_request_url);

  //std::cout<< "URL: " << client_request_url<< std::endl;

  if (fetch_cache == LRU_cache.cache_data.end()) {
    output_log({std::to_string(request_id), ": not in cache"});
    send_buffer(server_fd, request.request.c_str(), request.request.size(), 0);
    handle_get_response(client_fd, server_fd, request_id, request, LRU_cache);
  }
  else {
    Response cached_response = fetch_cache->second;
    // Cache control: No-Cache
    if (cached_response.header["CACHE-CONTROL"].find("no-cache") != std::string::npos) {
      output_log({std::to_string(request_id), ": in cache, requires validation"});
      // Revalidate
      revalidate(client_fd, server_fd, request_id, request, LRU_cache);
      //std::cout << "No-cache\n";
    }
    else {
      // Detect expire time
      if (isExpire(cached_response, LRU_cache)) {
        output_log({std::to_string(request_id),
                    ": in cache, but expired at ",
                    cached_response.header["DATE"]});
        // Revalidate
        revalidate(client_fd, server_fd, request_id, request, LRU_cache);
        //std::cout << "Expired!!\n";
      }
      else {
        output_log({std::to_string(request_id), ": in cache, valid"});
        reply_with_cache(client_fd, request, LRU_cache);
        //std::cout << "Fresh!!\n";
      }
    }
  }

  //std::cout << "Cache Exist\n";
}

void revalidate(int client_fd,
                int server_fd,
                int request_id,
                Request & request,
                Cache & LRU_cache) {
  Response cache_response = LRU_cache.cache_data[request.url];

  std::string request_new_header = request.request;
  // Etag
  if (cache_response.header.find("ETAG") != cache_response.header.end()) {
    std::string etag_resposne =
        "If-None-Match: " + cache_response.header["ETAG"] + "\r\n";

    //std::cout << "Old request: " << request.request << std::endl;
    request_new_header =
        request_new_header.insert(request_new_header.length() - 2, etag_resposne);
    //std::cout << "New request: " << request_new_header << std::endl;
  }
  // Last Modified
  else if (cache_response.header.find("LAST-MODIFIED") != cache_response.header.end()) {
    std::string lastmodify_resposne =
        "If-Modified-Since: " + cache_response.header["LAST-MODIFIED"] + "\r\n";

    //std::cout << "Old request last modify: " << request.request << std::endl;
    request_new_header =
        request_new_header.insert(request_new_header.length() - 2, lastmodify_resposne);
    //std::cout << "New request last modify: " << request_new_header << std::endl;
  }

  output_log({std::to_string(request_id),
              ": Requesting \"",
              request.start_line,
              "\" from ",
              request.hostname});

  // Send the right response to revalidate
  send_buffer(server_fd, request_new_header.c_str(), request_new_header.size(), 0);
  handle_revalidate_response(client_fd, server_fd, request_id, request, LRU_cache);
}

void handle_get_response(int client_fd,
                         int server_fd,
                         int request_id,
                         Request & request,
                         Cache & LRU_cache) {
  // Receive response from client
  std::string http_response;
  int nbytes;

  if ((nbytes = recv_http_message_header(server_fd, http_response, 0)) <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  // Parse the header
  HttpParser server_parser(http_response);

  // create a response object
  Response server_response = server_parser.parseResponse();

  // receive rest of the body
  nbytes = recv_http_message_body(server_fd,
                                  server_response.body,
                                  server_response.response,
                                  0,
                                  server_response.content_length);

  if (nbytes <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  output_log({std::to_string(request_id),
              ": Received \"",
              server_response.start_line,
              "\" from ",
              request.hostname});

  // Cache control: No-Store
  if (server_response.header["CACHE-CONTROL"].find("no-store") == std::string::npos) {
    LRU_cache.cache_saved_order.push_back(request.url);
    LRU_cache.cache_data[request.url] = server_response;

    //std::cout << "No no-store\n";
  }

  // std::cout << server_response.protocol_version <<std::endl;
  // std::cout << server_response.status_code <<std::endl;
  // std::cout << server_response.status_text <<std::endl;
  // std::cout << server_response.start_line <<std::endl;

  // for(auto it : server_response.header){
  //   std::cout << it.first << " " << it.second <<std::endl;
  // }

  output_log({std::to_string(request_id), ": Responding \"", server_response.start_line});

  send_buffer(
      client_fd, server_response.response.c_str(), server_response.response.size(), 0);

  // Chunked = -1
  // if(server_response.content_length == -1){
  //   send_buffer(client_fd, server_response.response.c_str(), server_response.response.size(), 0);

  //   std::cout << "Send Chunk " << server_response.response.size()<<std::endl;;
  // }
  // else{
  //   send_buffer(client_fd, server_response.response.c_str(), server_response.response.size(), 0);
  //   std::cout << "Send No Chunk\n";
  // }
}

void handle_revalidate_response(int client_fd,
                                int server_fd,
                                int request_id,
                                Request & request,
                                Cache & LRU_cache) {
  // Receive response from client
  std::string http_response;
  int nbytes;

  if ((nbytes = recv_http_message_header(server_fd, http_response, 0)) <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  // Parse the header
  HttpParser server_parser(http_response);

  // create a response object
  Response server_response = server_parser.parseResponse();

  // receive rest of the body
  nbytes = recv_http_message_body(server_fd,
                                  server_response.body,
                                  server_response.response,
                                  0,
                                  server_response.content_length);

  if (nbytes <= 0) {
    return;  // DANGER: Needs to check error handling
  }

  output_log({std::to_string(request_id),
              ": Received \"",
              server_response.start_line,
              "\" from ",
              request.hostname});

  //std::cout << "Server Response: \n";
  //std::cout << server_response.protocol_version << std::endl;
  //std::cout << server_response.status_code << std::endl;
  //std::cout << server_response.status_text << std::endl;
  //std::cout << server_response.start_line << std::endl;

  // 304 retrieve from cache
  if (server_response.status_code == "304") {
    output_log(
        {std::to_string(request_id), ": Responding \"HTTP/1.1 304 Not Modified\""});
    reply_with_cache(client_fd, request, LRU_cache);
    //std::cout << "304\n";
  }
  else if (server_response.status_code == "200") {
    output_log({std::to_string(request_id), ": Responding \"HTTP/1.1 200 OK\""});
    LRU_cache.cache_data[request.url] = server_response;
    reply_with_cache(client_fd, request, LRU_cache);
    //std::cout << "200\n";
  }
  else {
    output_log({std::to_string(request_id),
                ": Responding \"HTTP/1.1 500 Internal Server Error\""});
    send_buffer(
        client_fd, server_response.response.c_str(), server_response.response.size(), 0);
    //std::cout << "500\n";
  }
}

void reply_with_cache(int client_fd, Request & request, Cache & LRU_cache) {
  Response cache_response = LRU_cache.cache_data[request.url];

  // Implement LRU
  std::vector<std::string>::iterator it = find(LRU_cache.cache_saved_order.begin(),
                                               LRU_cache.cache_saved_order.end(),
                                               request.url);

  LRU_cache.cache_saved_order.erase(it);
  LRU_cache.cache_saved_order.push_back(request.url);

  send_buffer(
      client_fd, cache_response.response.c_str(), cache_response.response.size(), 0);
}

bool isExpire(Response & response, Cache & LRU_cache) {
  // Get current time
  time_t now = time(NULL);
  // Date
  struct tm * gen_date = gmtime(&now);
  if (response.header.find("DATE") != response.header.end()) {
    //std::cout << "Now: " << asctime(gen_date) << std::endl;

    strptime(response.header["DATE"].c_str(), "%a, %d %b %Y %T GMT", gen_date);
    //std::cout << "Date: " << asctime(gen_date) << std::endl;
  }

  // Age
  double age = 0;
  if (response.header.find("AGE") != response.header.end()) {
    age = stod(response.header["AGE"]);
    //std::cout << "Age: " << age << std::endl;
  }

  // Cache control: maxage
  size_t max_age_begin;
  const char * max_age_str = "max-age";
  size_t max_age_strlen = strlen(max_age_str) + 1;
  if ((max_age_begin = response.header["CACHE-CONTROL"].find(max_age_str)) !=
      std::string::npos) {
    double fresh_period = difftime(mktime(gmtime(&now)), mktime(gen_date));
    //std::cout << "Fresh period: " << fresh_period << std::endl;

    size_t mex_age_end =
        response.header["CACHE-CONTROL"].find(",", max_age_begin + max_age_strlen);
    std::string max_age = response.header["CACHE-CONTROL"].substr(
        max_age_begin + max_age_strlen, mex_age_end);
    double max_age_convert = stod(max_age) - age;

    //std::cout << "Max age: " << max_age_convert << std::endl;
    ;

    return max_age_convert < fresh_period;
  }

  // Expire Time
  struct tm expire_date;
  if (response.header.find("EXPIRES") != response.header.end()) {
    strptime(response.header["EXPIRES"].c_str(), "%a, %d %b %Y %T GMT", &expire_date);

    //std::cout << "Expires: " << asctime(&expire_date) << std::endl;

    return difftime(mktime(&expire_date), now) < 0;
  }

  return false;
}

void output_log(std::vector<std::string> record) {
  mtx.lock();
  for (auto t : record) {
    write_log << t;
  }
  write_log << std::endl;
  mtx.unlock();
}
