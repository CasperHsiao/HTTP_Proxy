#include "request.hpp"
#include "networks.hpp"

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

    cout << status << endl;
    cout << right_id_host<< endl;

    // Parse the header
    HttpParser client_parser(right_id_host);

    // create a request object
    Request client_request = client_parser.parseRequest();

    cout << client_request.hostname<< endl;
    cout << client_request.port<< endl;

    // build connection with original server
    int server_fd = get_connected_socket(client_request.hostname.c_str(), client_request.port.c_str());

    if(server_fd < 0){
        cerr << "Error: cannot connect to server's socket" << endl;
        exit(EXIT_FAILURE);
    }

    // Seperate function calls according to method
    if(client_request.method == "CONNECT"){
        handle_connect_request(coonection_fd, server_fd, client_request);
    }
    else if(client_request.method == "POST"){
        cout<<"Post\n";
    }
    else if(client_request.method == "GET"){
        cout<<"Get\n";
    }
    // Shit down both client and server's socket connections
    close(listener_fd);
    close(listener_fd);
}