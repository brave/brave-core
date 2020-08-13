/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_INFO_H_

#include <stdint.h>
#include <string>
#include <vector>

namespace brave_rewards {

struct PublisherInfo {
  PublisherInfo();
  ~PublisherInfo();
  PublisherInfo(const PublisherInfo& info);

  std::string id;
  uint64_t duration = 0;
  double score = 0.0;
  uint32_t visits = 0;
  uint32_t percent = 0;
  double weight = 0.0;
  uint32_t excluded = 0;
  uint64_t reconcile_stamp = 0;
  uint32_t status = 0;
  uint64_t status_updated_at = 0;
  std::string name;
  std::string url;
  std::string provider;
  std::string favicon_url;
};

typedef std::vector<PublisherInfo> PublisherInfoList;

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_INFO_H_
