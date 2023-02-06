/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/targeting/models/behavioral/bandits/epsilon_greedy_bandit_model.h"

#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_map.h"
#include "base/rand_util.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/features/epsilon_greedy_bandit_features.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_util.h"
#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms_alias.h"
#include "bat/ads/internal/resources/behavioral/bandits/epsilon_greedy_bandit_resource_util.h"

namespace ads::targeting::model {

namespace {

constexpr size_t kTopArmCount = 3;

using ArmBucketMap =
    base::flat_map<double, std::vector<EpsilonGreedyBanditArmInfo>>;
using ArmList = std::vector<EpsilonGreedyBanditArmInfo>;
using ArmBucketPair = std::pair<double, ArmList>;
using ArmBucketList = std::vector<ArmBucketPair>;

SegmentList ToSegmentList(const ArmList& arms) {
  SegmentList segments;

  for (const auto& arm : arms) {
    segments.push_back(arm.segment);
  }

  return segments;
}

ArmList ToArmList(const EpsilonGreedyBanditArmMap& arms) {
  ArmList arm_list;

  for (const auto& arm : arms) {
    arm_list.push_back(arm.second);
  }

  return arm_list;
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
  const SegmentList segments =
      resource::GetEpsilonGreedyBanditEligibleSegments();
  if (segments.empty()) {
    return {};
  }

  EpsilonGreedyBanditArmMap eligible_arms;

  for (const auto& arm : arms) {
    if (!base::Contains(segments, arm.first)) {
      continue;
    }

    eligible_arms[arm.first] = arm.second;
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

  for (const auto& bucket : buckets) {
    const size_t available_arms = count - top_arms.size();
    if (available_arms < 1) {
      return top_arms;
    }

    ArmList arms = bucket.second;
    if (arms.size() > available_arms) {
      // Sample without replacement
      base::RandomShuffle(std::begin(arms), std::end(arms));
      arms.resize(available_arms);
    }

    top_arms.insert(top_arms.cend(), arms.cbegin(), arms.cend());
  }

  return top_arms;
}

SegmentList ExploreSegments(const EpsilonGreedyBanditArmMap& arms) {
  SegmentList segments;

  for (const auto& arm : arms) {
    segments.push_back(arm.first);
  }

  if (segments.size() > kTopArmCount) {
    base::RandomShuffle(std::begin(segments), std::end(segments));
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

  if (base::RandDouble() < features::GetEpsilonGreedyBanditEpsilonValue()) {
    segments = ExploreSegments(eligible_arms);
  } else {
    segments = ExploitSegments(eligible_arms);
  }

  return segments;
}

}  // namespace

SegmentList EpsilonGreedyBandit::GetSegments() const {
  return GetSegmentsForArms(GetEpsilonGreedyBanditArms());
}

}  // namespace ads::targeting::model
