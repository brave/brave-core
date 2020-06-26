#include "brave/components/brave_shields/common/block_decision.h"

namespace brave_shields {

bool BlockDecision::IsAdBlockDecision() const {
  return false;
}

bool BlockDecision::IsTrackerBlockDecision() const {
  return false;
}

bool BlockDecision::IsHTTPUpgradableResourceBlockDecision() const {
  return false;
}

AdBlockDecision::AdBlockDecision(const std::string& rule)
  : rule_(rule) {}

TrackerBlockDecision::TrackerBlockDecision(const std::string& host)
  : host_(host) {}

HTTPUpgradableResourceBlockDecision::HTTPUpgradableResourceBlockDecision() {}

BlockDecision::~BlockDecision() {}

AdBlockDecision::~AdBlockDecision() {}

TrackerBlockDecision::~TrackerBlockDecision() {}

HTTPUpgradableResourceBlockDecision::~HTTPUpgradableResourceBlockDecision() {}

bool AdBlockDecision::IsAdBlockDecision() const {
  return true;
}

bool TrackerBlockDecision::IsTrackerBlockDecision() const {
  return true;
}

bool HTTPUpgradableResourceBlockDecision::IsHTTPUpgradableResourceBlockDecision() const {
  return true;
}

const char* AdBlockDecision::BlockType() const {
  return kAds;
}

const char* TrackerBlockDecision::BlockType() const {
  return kTrackers;
}

const char* HTTPUpgradableResourceBlockDecision::BlockType() const {
  return kHTTPUpgradableResources;
}

}  // namespace brave_shields
