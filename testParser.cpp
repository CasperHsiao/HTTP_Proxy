#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "httpParser.hpp"

using namespace std;

int main(int argc, char ** argv) {
  ifstream f(argv[1], ifstream::in);
  if (!f.is_open()) {
    cerr << "Unable to open the file!\n" << endl;
    return EXIT_FAILURE;
  }
  string str =
      string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  cout << str << endl;
  HttpParser parser(str);
  Request req = parser.parseRequest();
  cout << req.start_line << "()" << endl;  // "()" denotes end of line for display purpose
  unordered_map<string, string> mymap = req.header;
  for (auto it = mymap.begin(); it != mymap.end(); ++it) {
    cout << it->first << ":" << it->second << "()" << endl;
  }
  cout << req.body << "()" << endl;
  cout << "Request object fields" << endl;

  cout << req.method << "()" << endl;
  cout << req.url << "()" << endl;
  cout << req.protocol_version << "()" << endl;
  cout << req.hostname << "()" << endl;
  cout << req.port << "()" << endl;

  return EXIT_SUCCESS;
}
