/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PENDING_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PENDING_CONTRIBUTION_H_

#include <string>
#include <vector>

namespace brave_rewards {

struct PendingContributionInfo {
  PendingContributionInfo();
  ~PendingContributionInfo();
  PendingContributionInfo(const PendingContributionInfo& data);

  std::string publisher_key;
  uint32_t status;
  std::string name;
  std::string favicon_url;
  std::string url;
  std::string provider;
  double amount = 0;
  uint32_t added_date = 0;
  std::string viewing_id;
  int32_t type;
  uint32_t expiration_date = 0;
};

using PendingContributionInfoList = std::vector<PendingContributionInfo>;

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PENDING_CONTRIBUTION_H_
