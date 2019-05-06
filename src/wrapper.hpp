#ifndef ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
#define ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_

#include <memory>
#include <string>

extern "C" {
#include "lib.h"
}

namespace adblock {

class Blocker {
 public:
  Blocker(const std::string& rules);
  bool Matches(const std::string& url, const std::string& tab_url,
      const std::string& resource_type);
  ~Blocker();

 private:
  Blocker(const Blocker&) = delete;
  void operator=(const Blocker&) = delete;
  C_Blocker *raw;
};

}  // namespace adblock

#endif  // ADBLOCK_RUST_FFI_SRC_WRAPPER_HPP_
