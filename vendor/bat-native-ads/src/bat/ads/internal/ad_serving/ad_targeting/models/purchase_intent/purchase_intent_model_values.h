/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_PURCHASE_INTENT_PURCHASE_INTENT_MODEL_VALUES_H_  // NOLINT
#define BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_PURCHASE_INTENT_PURCHASE_INTENT_MODEL_VALUES_H_  // NOLINT

#include "base/time/time.h"
#include "bat/ads/internal/ad_serving/ad_targeting/models/model.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_aliases.h"

namespace ads {
namespace ad_targeting {
namespace model {

const uint16_t kSignalLevel = 1;
const uint16_t kThreshold = 3;
const int64_t kTimeWindowInSeconds = 7 * (24 * base::Time::kSecondsPerHour);
const size_t kMaximumSegments = 3;

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_SERVING_AD_TARGETING_MODELS_PURCHASE_INTENT_PURCHASE_INTENT_MODEL_VALUES_H_  // NOLINT
