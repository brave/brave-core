/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_BANNER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_BANNER_H_

#include <string>
#include <vector>
#include <map>

namespace brave_rewards {

struct PublisherBanner {
  PublisherBanner();
  ~PublisherBanner();
  PublisherBanner(const PublisherBanner& properties);

  std::string publisher_key;
  std::string title;
  std::string name;
  std::string description;
  std::string background;
  std::string logo;
  std::vector<double> amounts;
  std::string provider;
  std::map<std::string, std::string> links;
  uint32_t status;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_BANNER_H_
