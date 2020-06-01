/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_parameters.h"

namespace brave_rewards {

RewardsParameters::RewardsParameters() = default;
RewardsParameters::~RewardsParameters() = default;

RewardsParameters::RewardsParameters(const RewardsParameters &properties) {
  rate = properties.rate;
  auto_contribute_choice = properties.auto_contribute_choice;
  auto_contribute_choices = properties.auto_contribute_choices;
  tip_choices = properties.tip_choices;
  monthly_tip_choices = properties.monthly_tip_choices;
}

}  // namespace brave_rewards
