#include "request.hpp"
#include "httpParser.hpp"

using namespace std;

#define HEADER_LEN 70000

void handle_request(int listener_fd, int coonection_fd){
    // Receive header from client
    char right_id_host[HEADER_LEN];
    int status = recv(coonection_fd, &right_id_host, HEADER_LEN, MSG_WAITALL);

    if(status < 0){
        cerr << "Error: cannot connect to client's socket" << endl;
        exit(EXIT_FAILURE);
    }

    // cout<< status << endl;
    // cout<< right_id_host;

    // Parse the header
    HttpParser client_parser(right_id_host);

    // create a request object
    Request client_request = client_parser.parseRequest();

    // Seperate function calls according to method
    if(client_request.method == "CONNECT"){
        cout<<"Connect\n";
    }
    else if(client_request.method == "POST"){
        cout<<"Post\n";
    }
    else if(client_request.method == "GET"){
        cout<<"Get\n";
    }
    // Shit down both client and server's socket connections
    close(listener_fd);
}