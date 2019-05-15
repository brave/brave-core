#include "wrapper.hpp"

extern "C" {
#include "lib.h"
}

namespace adblock {

Engine::Engine(const std::string& rules) : raw(engine_create(rules.c_str())) {
}

bool Engine::Matches(const std::string& url, const std::string& host,
    const std::string& tab_host, bool is_third_party,
    const std::string& resource_type) {
  return engine_match(raw, url.c_str(), host.c_str(),tab_host.c_str(),
      is_third_party, resource_type.c_str());
}

bool Engine::Deserialize(const char* serialized_data) {
  return engine_deserialize(raw, serialized_data);
}

Engine::~Engine() {
  engine_destroy(raw);
}

}  // namespace adblock
