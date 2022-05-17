/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_MODELS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_MODEL_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_MODELS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_MODEL_UNITTEST_UTIL_H_

#include "bat/ads/internal/segments/segments_aliases.h"

namespace ads {
namespace targeting {
namespace model {

void SaveSegments(const SegmentList& segments);

void SaveAllSegments();

}  // namespace model
}  // namespace targeting
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SERVING_TARGETING_MODELS_BEHAVIORAL_BANDITS_EPSILON_GREEDY_BANDIT_MODEL_UNITTEST_UTIL_H_
