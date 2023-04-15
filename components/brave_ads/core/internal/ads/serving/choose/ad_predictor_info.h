/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_AD_PREDICTOR_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_AD_PREDICTOR_INFO_H_

#include <map>
#include <string>

#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"

namespace brave_ads {

template <typename T>
struct AdPredictorInfo final {
  T creative_ad;
  SegmentList segments;
  bool does_match_intent_child_segments = false;
  bool does_match_intent_parent_segments = false;
  bool does_match_interest_child_segments = false;
  bool does_match_interest_parent_segments = false;
  int ad_last_seen_hours_ago = 0;
  int advertiser_last_seen_hours_ago = 0;
  double score = 0.0;
};

template <typename T>
using CreativeAdPredictorMap = std::map</*creative_instance_id*/ std::string,
                                        /*creative_ad*/ AdPredictorInfo<T>>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_SERVING_CHOOSE_AD_PREDICTOR_INFO_H_
