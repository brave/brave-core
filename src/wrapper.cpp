#include "wrapper.hpp"

extern "C" {
#include "lib.h"
}

namespace adblock {

Blocker::Blocker(const std::string& rules) : raw(blocker_create(rules.c_str())) {
}

bool Blocker::Matches(const std::string& url, const std::string& tab_url,
    const std::string& resource_type) {
  return blocker_match(raw, url.c_str(), tab_url.c_str(), resource_type.c_str());
}

Blocker::~Blocker() {
  blocker_destroy(raw);
}

}  // namespace adblock
