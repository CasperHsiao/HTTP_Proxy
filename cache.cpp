#include "request.hpp"
#include "networks.hpp"
#include "cache.hpp"

#include <string>

using namespace std;

// void handle_get_request(int client_fd, int server_fd, Request & request, Cache & LRU_cache){
//     string client_request_url = request.url;
//     unordered_map<std::string, std::string>::const_iterator fetch_cache = LRU_cache.cache_data.find(client_request_url);

//     if(fetch_cache == LRU_cache.cache_data.end()){
//         cout<< "Fetch new request\n";
//     }
//     else{
//         cout<< "Cache Exist\n";
//     }
// }