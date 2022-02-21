#ifndef __CACEH_H__
#define __CACEH_H__

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "response.hpp"

class Cache {
 public:
  std::unordered_map<std::string, Response> cache_data;
  std::vector<std::string> cache_saved_order;
};

#endif
