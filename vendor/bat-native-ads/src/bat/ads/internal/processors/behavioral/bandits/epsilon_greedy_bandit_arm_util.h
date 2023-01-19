/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_UTIL_H_

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms_alias.h"

namespace ads::targeting {

void SetEpsilonGreedyBanditArms(const EpsilonGreedyBanditArmMap& arms);
EpsilonGreedyBanditArmMap GetEpsilonGreedyBanditArms();

}  // namespace ads::targeting

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_ARM_UTIL_H_
