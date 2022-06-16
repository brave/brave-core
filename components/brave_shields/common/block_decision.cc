/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

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

AdBlockDecision::AdBlockDecision(const std::string& rule) : rule_(rule) {}

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

bool HTTPUpgradableResourceBlockDecision::
    IsHTTPUpgradableResourceBlockDecision() const {
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
