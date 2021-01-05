/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"

#include <stdint.h>

#include <algorithm>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/rand_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/features/bandits/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace ad_targeting {
namespace model {

namespace {

const size_t kTopArmCount = 3;

using ArmBucketMap = std::map<double, std::vector<EpsilonGreedyBanditArmInfo>>;
using ArmList = std::vector<EpsilonGreedyBanditArmInfo>;
using ArmBucketPair = std::pair<double, ArmList>;
using ArmBucketList = std::vector<ArmBucketPair>;

SegmentList ToSegmentList(
    const ArmList& arms) {
  SegmentList segments;

  for (const auto& arm : arms) {
    segments.push_back(arm.segment);
  }

  return segments;
}

ArmList ToArmList(
    const EpsilonGreedyBanditArmMap& arms) {
  ArmList arm_list;

  for (const auto& arm : arms) {
    arm_list.push_back(arm.second);
  }

  return arm_list;
}

ArmBucketMap BucketSortArms(
    const ArmList& arms) {
  ArmBucketMap buckets;

  for (const auto& arm : arms) {
    const auto iter = buckets.find(arm.value);
    if (iter == buckets.end()) {
      buckets.insert({arm.value, {arm}});
      continue;
    }

    iter->second.push_back(arm);
  }

  return buckets;
}

ArmBucketList GetSortedBuckets(
    const ArmBucketMap& arms) {
  const ArmBucketList unsorted_buckets{arms.begin(), arms.end()};
  ArmBucketList sorted_buckets(arms.size());
  std::partial_sort_copy(unsorted_buckets.begin(), unsorted_buckets.end(),
      sorted_buckets.begin(), sorted_buckets.end(), [](
          const ArmBucketPair& lhs, const ArmBucketPair& rhs) {
    return lhs.first > rhs.first;
  });

  return sorted_buckets;
}

ArmList GetTopArms(
    const ArmBucketList& buckets,
    const size_t count) {
  ArmList top_arms;

  for (const auto& bucket : buckets) {
    const size_t available_arms = count - top_arms.size();
    if (available_arms < 1) {
      return top_arms;
    }

    ArmList arms = bucket.second;
    if (arms.size() > available_arms) {
      // Sample without replacement
      base::RandomShuffle(begin(arms), end(arms));
      arms.resize(available_arms);
    }

    top_arms.insert(top_arms.end(), arms.begin(), arms.end());
  }

  return top_arms;
}

SegmentList ExploreSegments(
    const EpsilonGreedyBanditArmMap& arms) {
  SegmentList segments;

  for (const auto& arm : arms) {
    segments.push_back(arm.first);
  }

  base::RandomShuffle(begin(segments), end(segments));
  segments.resize(kTopArmCount);
  return segments;
}

SegmentList ExploitSegments(
    const EpsilonGreedyBanditArmMap& arms) {
  const ArmList arm_list = ToArmList(arms);
  const ArmBucketMap unsorted_buckets = BucketSortArms(arm_list);
  const ArmBucketList sorted_buckets = GetSortedBuckets(unsorted_buckets);
  const ArmList top_arms = GetTopArms(sorted_buckets, kTopArmCount);
  return ToSegmentList(top_arms);
}

}  // namespace

EpsilonGreedyBandit::EpsilonGreedyBandit() = default;

EpsilonGreedyBandit::~EpsilonGreedyBandit() = default;

SegmentList EpsilonGreedyBandit::GetSegments() const {
  const std::string json = AdsClientHelper::Get()->GetStringPref(
      prefs::kEpsilonGreedyBanditArms);
  const EpsilonGreedyBanditArmMap arms =
      EpsilonGreedyBanditArms::FromJson(json);
  return ChooseAction(arms);
}

///////////////////////////////////////////////////////////////////////////////

SegmentList EpsilonGreedyBandit::ChooseAction(
    const EpsilonGreedyBanditArmMap& arms) const {
  SegmentList segments;

  if (arms.size() < kTopArmCount) {
    return segments;
  }

  const double epsilon = features::GetEpsilonGreedyBanditEpsilonValue();
  if (base::RandDouble() < epsilon) {
    return ExploreSegments(arms);
  }

  return ExploitSegments(arms);
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
