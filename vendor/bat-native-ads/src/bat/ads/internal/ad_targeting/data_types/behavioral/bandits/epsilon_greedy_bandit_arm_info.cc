/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_arm_info.h"

namespace ads {
namespace ad_targeting {

EpsilonGreedyBanditArmInfo::EpsilonGreedyBanditArmInfo() = default;

EpsilonGreedyBanditArmInfo::EpsilonGreedyBanditArmInfo(
    const EpsilonGreedyBanditArmInfo& info) = default;

EpsilonGreedyBanditArmInfo::~EpsilonGreedyBanditArmInfo() = default;

bool EpsilonGreedyBanditArmInfo::operator==(
    const EpsilonGreedyBanditArmInfo& rhs) const {
  return segment == rhs.segment && value == rhs.value && pulls == rhs.pulls;
}

bool EpsilonGreedyBanditArmInfo::operator!=(
    const EpsilonGreedyBanditArmInfo& rhs) const {
  return !(*this == rhs);
}

bool EpsilonGreedyBanditArmInfo::IsValid() const {
  if (segment.empty() || value < 0 || value > 1.0 || pulls < 0) {
    return false;
  }

  return true;
}

}  // namespace ad_targeting
}  // namespace ads
