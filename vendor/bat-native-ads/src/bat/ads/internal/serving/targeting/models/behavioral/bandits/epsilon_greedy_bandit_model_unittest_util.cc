/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/targeting/models/behavioral/bandits/epsilon_greedy_bandit_model_unittest_util.h"

#include <string>

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/segments/segments_json_writer.h"
#include "bat/ads/internal/targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_segments.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace targeting {
namespace model {

void SaveSegments(const SegmentList& segments) {
  const std::string json = JSONWriter::WriteSegments(segments);

  AdsClientHelper::Get()->SetStringPref(
      prefs::kEpsilonGreedyBanditEligibleSegments, json);
}

void SaveAllSegments() {
  SaveSegments(kSegments);
}

}  // namespace model
}  // namespace targeting
}  // namespace ads
