/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PARAMETERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PARAMETERS_H_

#include <string>
#include <map>
#include <vector>

namespace brave_rewards {

struct RewardsParameters {
  RewardsParameters();
  ~RewardsParameters();
  RewardsParameters(const RewardsParameters& properties);

  double rate;
  double auto_contribute_choice;
  std::vector<double> auto_contribute_choices;
  std::vector<double> tip_choices;
  std::vector<double> monthly_tip_choices;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PARAMETERS_H_
