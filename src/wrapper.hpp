#ifndef ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
#define ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_

#include <memory>
#include <string>

extern "C" {
#include "lib.h"
}

namespace adblock {

class Engine {
 public:
  Engine(const std::string& rules);
  bool Matches(const std::string& url, const std::string& host,
      const std::string& tab_host, bool is_third_party,
      const std::string& resource_type);
  bool Deserialize(const char* serialized_data);
  void AddTag(const std::string& tag);
  void RemoveTag(const std::string& tag);
  ~Engine();

 private:
  Engine(const Engine&) = delete;
  void operator=(const Engine&) = delete;
  C_Engine* raw;
};

}  // namespace adblock

#endif  // ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
