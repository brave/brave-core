/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_PUBLISHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_PUBLISHER_H_

#include <string>

namespace brave_rewards {

struct ContributionPublisher {
  ContributionPublisher();
  ~ContributionPublisher();
  ContributionPublisher(const ContributionPublisher& info);

  std::string contribution_id;
  std::string publisher_key;
  double total_amount;
  double contributed_amount;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_CONTRIBUTION_PUBLISHER_H_
