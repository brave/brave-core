#include "wrapper.hpp"

extern "C" {
#include "lib.h"
}

namespace adblock {

Engine::Engine(const std::string& rules) : raw(engine_create(rules.c_str())) {
}

bool Engine::Matches(const std::string& url, const std::string& host,
    const std::string& tab_host, bool is_third_party,
    const std::string& resource_type, bool* explicit_cancel,
    bool* saved_from_exception) {
  return engine_match(raw, url.c_str(), host.c_str(),tab_host.c_str(),
      is_third_party, resource_type.c_str(), explicit_cancel,
      saved_from_exception);
}

bool Engine::Deserialize(const char* serialized_data) {
  return engine_deserialize(raw, serialized_data);
}

void Engine::AddTag(const std::string& tag) {
  engine_add_tag(raw, tag.c_str());
}

void Engine::RemoveTag(const std::string& tag) {
  engine_remove_tag(raw, tag.c_str());
}

Engine::~Engine() {
  engine_destroy(raw);
}

}  // namespace adblock
