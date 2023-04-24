/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_model.h"

#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_feature.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_util.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arms_alias.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/multi_armed_bandits/epsilon_greedy_bandit_resource_util.h"

namespace brave_ads {

namespace {

using ArmBucketMap =
    base::flat_map</*value*/ double, std::vector<EpsilonGreedyBanditArmInfo>>;
using ArmList = std::vector<EpsilonGreedyBanditArmInfo>;
using ArmBucketPair = std::pair</*value*/ double, ArmList>;
using ArmBucketList = std::vector<ArmBucketPair>;

constexpr size_t kTopArmCount = 3;

SegmentList ToSegmentList(const ArmList& arms) {
  SegmentList segments;

  for (const auto& arm : arms) {
    segments.push_back(arm.segment);
  }

  return segments;
}

ArmList ToArmList(const EpsilonGreedyBanditArmMap& arms) {
  ArmList list;

  for (const auto& [segment, arm] : arms) {
    list.push_back(arm);
  }

  return list;
}

ArmBucketMap BucketSortArms(const ArmList& arms) {
  ArmBucketMap buckets;

  for (const auto& arm : arms) {
    const auto iter = buckets.find(arm.value);
    if (iter == buckets.cend()) {
      buckets.insert({arm.value, {arm}});
      continue;
    }

    iter->second.push_back(arm);
  }

  return buckets;
}

EpsilonGreedyBanditArmMap GetEligibleArms(
    const EpsilonGreedyBanditArmMap& arms) {
  const SegmentList segments = GetEpsilonGreedyBanditEligibleSegments();
  if (segments.empty()) {
    return {};
  }

  EpsilonGreedyBanditArmMap eligible_arms;

  for (const auto& [segment, arm] : arms) {
    if (base::Contains(segments, segment)) {
      eligible_arms[segment] = arm;
    }
  }

  return eligible_arms;
}

ArmBucketList GetSortedBuckets(const ArmBucketMap& arms) {
  const ArmBucketList unsorted_buckets{arms.cbegin(), arms.cend()};
  ArmBucketList sorted_buckets(arms.size());
  base::ranges::partial_sort_copy(
      unsorted_buckets, sorted_buckets,
      [](const ArmBucketPair& lhs, const ArmBucketPair& rhs) {
        return lhs.first > rhs.first;
      });

  return sorted_buckets;
}

ArmList GetTopArms(const ArmBucketList& buckets, const size_t count) {
  ArmList top_arms;

  for (auto [value, arms] : buckets) {
    const size_t available_arms = count - top_arms.size();
    if (available_arms < 1) {
      return top_arms;
    }

    if (arms.size() > available_arms) {
      // Sample without replacement
      base::RandomShuffle(arms.begin(), arms.end());
      arms.resize(available_arms);
    }

    base::Extend(top_arms, arms);
  }

  return top_arms;
}

SegmentList ExploreSegments(const EpsilonGreedyBanditArmMap& arms) {
  SegmentList segments;

  for (const auto& [segment, arm] : arms) {
    segments.push_back(segment);
  }

  if (segments.size() > kTopArmCount) {
    base::RandomShuffle(segments.begin(), segments.end());
    segments.resize(kTopArmCount);
  }

  BLOG(2, "Exploring epsilon greedy bandit segments:");
  for (const auto& segment : segments) {
    BLOG(2, "  " << segment);
  }

  return segments;
}

SegmentList ExploitSegments(const EpsilonGreedyBanditArmMap& arms) {
  const ArmBucketMap unsorted_buckets = BucketSortArms(ToArmList(arms));
  const ArmBucketList sorted_buckets = GetSortedBuckets(unsorted_buckets);
  const ArmList top_arms = GetTopArms(sorted_buckets, kTopArmCount);
  SegmentList segments = ToSegmentList(top_arms);

  BLOG(2, "Exploiting epsilon greedy bandit segments:");
  for (const auto& segment : segments) {
    BLOG(2, "  " << segment);
  }

  return segments;
}

SegmentList GetSegmentsForArms(const EpsilonGreedyBanditArmMap& arms) {
  SegmentList segments;

  if (arms.size() < kTopArmCount) {
    return segments;
  }

  const EpsilonGreedyBanditArmMap eligible_arms = GetEligibleArms(arms);

  if (base::RandDouble() < kEpsilonGreedyBanditEpsilonValue.Get()) {
    segments = ExploreSegments(eligible_arms);
  } else {
    segments = ExploitSegments(eligible_arms);
  }

  return segments;
}

}  // namespace

SegmentList EpsilonGreedyBanditModel::GetSegments() const {
  return GetSegmentsForArms(GetEpsilonGreedyBanditArms());
}

}  // namespace brave_ads
