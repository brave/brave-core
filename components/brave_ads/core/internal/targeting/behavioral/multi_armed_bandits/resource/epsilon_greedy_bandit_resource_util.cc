/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/resource/epsilon_greedy_bandit_resource_util.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/segments/segment_value_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

void ResetEpsilonGreedyBanditEligibleSegments() {
  AdsClientHelper::GetInstance()->ClearPref(
      prefs::kEpsilonGreedyBanditEligibleSegments);
}

void SetEpsilonGreedyBanditEligibleSegments(const SegmentList& segments) {
  AdsClientHelper::GetInstance()->SetListPref(
      prefs::kEpsilonGreedyBanditEligibleSegments, SegmentsToValue(segments));
}

SegmentList GetEpsilonGreedyBanditEligibleSegments() {
  const absl::optional<base::Value::List> list =
      AdsClientHelper::GetInstance()->GetListPref(
          prefs::kEpsilonGreedyBanditEligibleSegments);
  if (!list) {
    return {};
  }

  return SegmentsFromValue(*list);
}

}  // namespace brave_ads
