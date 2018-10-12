/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/browser/publisher_banner.h"

#include <string>
#include <vector>
#include <map>

namespace brave_rewards {

  PublisherBanner::PublisherBanner() :
    title(""),
    description(""),
    background(""),
    logo(""),
    amounts(std::vector<int>()),
    social(std::map<std::string, std::string>()) {}

  PublisherBanner::~PublisherBanner() { }

  PublisherBanner::PublisherBanner(const PublisherBanner &properties) {
    title = properties.title;
    description = properties.description;
    background = properties.background;
    logo = properties.logo;
    amounts = properties.amounts;
    social = properties.social;
  }

}  // namespace brave_rewards
