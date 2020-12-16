/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/rand_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {
const size_t kMaximumSegments = 3;
const double kEpsilon = 0.2;  // TOOD(Moritz Haller) add as param to feature
// const int kIntMax = std::numeric_limits<int>::max();
}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() = default;

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

SegmentList EpsilonGreedyBandit::GetSegments() const {
  const std::string json = AdsClientHelper::Get()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);
  const EpsilonGreedyBanditArmList arms =
      EpsilonGreedyBanditArms::FromJson(json);
  SegmentList segments = ChooseAction(arms);
  return segments;
}

///////////////////////////////////////////////////////////////////////////////

SegmentList EpsilonGreedyBandit::ChooseAction(
    const EpsilonGreedyBanditArmList& arms) const {
  SegmentList segments;

  // Explore
  if (base::RandDouble() < kEpsilon) {
    BLOG(1, "Explore with eps=" << kEpsilon);
    for (const auto& arm : arms) {
      segments.push_back(arm.segment);
    }

    // Sample without replacement
    base::RandomShuffle(begin(segments), end(segments));
    segments.resize(kMaximumSegments);
    return segments;
  }

  // Exploit
  BLOG(1, "Exploit with 1-eps=" << 1-kEpsilon);
  EpsilonGreedyBanditArmList arms_sorted(kMaximumSegments);
  std::partial_sort_copy(arms.begin(), arms.end(), arms_sorted.begin(),
      arms_sorted.end(), [](const EpsilonGreedyBanditArmInfo& l,
          const EpsilonGreedyBanditArmInfo& r) {
    // TOOD(Moritz Haller): Break ties randomly
    // if (l.value == r.value) {
      // return base::RandInt(0, kIntMax) % 2 == 1;
    // }
    return l.value > r.value;
  });

  for (const auto& arm : arms_sorted) {
    segments.push_back(arm.segment);
  }
  return segments;
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
