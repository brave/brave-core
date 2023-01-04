/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource_util.h"

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/segments/segment_value_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::resource {

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

}  // namespace ads::resource
