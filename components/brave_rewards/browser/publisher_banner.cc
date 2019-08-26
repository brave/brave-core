/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_banner.h"

#include <string>
#include <vector>
#include <map>

namespace brave_rewards {

  PublisherBanner::PublisherBanner() :
    publisher_key(""),
    title(""),
    name(""),
    description(""),
    background(""),
    logo(""),
    amounts(std::vector<double>()),
    provider(""),
    links(std::map<std::string, std::string>()),
    status(0) {}

  PublisherBanner::~PublisherBanner() { }

  PublisherBanner::PublisherBanner(const PublisherBanner &properties) {
    publisher_key = properties.publisher_key;
    title = properties.title;
    name = properties.name;
    description = properties.description;
    background = properties.background;
    logo = properties.logo;
    amounts = properties.amounts;
    provider = properties.provider;
    links = properties.links;
    status = properties.status;
  }

}  // namespace brave_rewards
