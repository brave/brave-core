/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit_arm_info.h"

namespace ads {

EpsilonGreedyBanditArmInfo::EpsilonGreedyBanditArmInfo() = default;

EpsilonGreedyBanditArmInfo::~EpsilonGreedyBanditArmInfo() = default;

bool EpsilonGreedyBanditArmInfo::IsValid() const {
  if (segment.empty() || value < 0 || value > 1.0 || pulls < 0) {
    return false;
  }

  return true;
}

}  // namespace ads
