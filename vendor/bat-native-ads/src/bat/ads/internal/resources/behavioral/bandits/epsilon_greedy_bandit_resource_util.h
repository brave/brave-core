/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_UTIL_H_

#include "bat/ads/internal/segments/segment_alias.h"

namespace ads::resource {

void SetEpsilonGreedyBanditEligibleSegments(const SegmentList& segments);
SegmentList GetEpsilonGreedyBanditEligibleSegments();

}  // namespace ads::resource

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_RESOURCES_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_RESOURCE_UTIL_H_
