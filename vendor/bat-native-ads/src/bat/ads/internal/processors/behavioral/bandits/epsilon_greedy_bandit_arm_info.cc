/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads::targeting {

bool EpsilonGreedyBanditArmInfo::operator==(
    const EpsilonGreedyBanditArmInfo& other) const {
  return segment == other.segment && DoubleEquals(value, other.value) &&
         pulls == other.pulls;
}

bool EpsilonGreedyBanditArmInfo::operator!=(
    const EpsilonGreedyBanditArmInfo& other) const {
  return !(*this == other);
}

bool EpsilonGreedyBanditArmInfo::IsValid() const {
  return !segment.empty() && value >= 0.0 && value <= 1.0 && pulls >= 0;
}

}  // namespace ads::targeting
