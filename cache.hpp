#ifndef __CACEH_H__
#define __CACEH_H__

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

<<<<<<< HEAD
#include "response.hpp"

class Cache {
 public:
  std::unordered_map<std::string, Response> cache_data;
=======
class Cache {
 public:
  std::unordered_map<std::string, std::string> cache_data;
>>>>>>> 967bcffcb4add9e7b40b2383811f0f86868b433d
  std::vector<std::string> cache_saved_order;
};

#endif
