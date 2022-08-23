/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_info.h"

#include "bat/ads/internal/base/numbers/number_util.h"

namespace ads {
namespace targeting {

EpsilonGreedyBanditArmInfo::EpsilonGreedyBanditArmInfo() = default;

EpsilonGreedyBanditArmInfo::EpsilonGreedyBanditArmInfo(
    const EpsilonGreedyBanditArmInfo& info) = default;

EpsilonGreedyBanditArmInfo& EpsilonGreedyBanditArmInfo::operator=(
    const EpsilonGreedyBanditArmInfo& info) = default;

EpsilonGreedyBanditArmInfo::~EpsilonGreedyBanditArmInfo() = default;

bool EpsilonGreedyBanditArmInfo::operator==(
    const EpsilonGreedyBanditArmInfo& rhs) const {
  return segment == rhs.segment && DoubleEquals(value, rhs.value) &&
         pulls == rhs.pulls;
}

bool EpsilonGreedyBanditArmInfo::operator!=(
    const EpsilonGreedyBanditArmInfo& rhs) const {
  return !(*this == rhs);
}

bool EpsilonGreedyBanditArmInfo::IsValid() const {
  return !(segment.empty() || value < 0.0 || value > 1.0 || pulls < 0);
}

}  // namespace targeting
}  // namespace ads
