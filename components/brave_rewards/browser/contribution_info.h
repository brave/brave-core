/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_INFO_H_

#include <string>

namespace brave_rewards {

struct ContributionInfo {
  ContributionInfo();
  ~ContributionInfo();
  ContributionInfo(const ContributionInfo& properties);

  std::string probi;
  int month;
  int year;
  int type;
  uint32_t date = 0;
  std::string publisher_key;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_INFO_H_
