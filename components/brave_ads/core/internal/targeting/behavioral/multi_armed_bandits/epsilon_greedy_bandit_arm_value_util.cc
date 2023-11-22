/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/multi_armed_bandits/epsilon_greedy_bandit_arm_value_util.h"

#include <string>

#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr char kSegmentKey[] = "segment";
constexpr char kValueKey[] = "value";
constexpr char kPullsKey[] = "pulls";

absl::optional<EpsilonGreedyBanditArmInfo> MaybeGetArmFromDict(
    const base::Value::Dict& dict) {
  const std::string* const segment = dict.FindString(kSegmentKey);
  if (!segment || segment->empty()) {
    return absl::nullopt;
  }

  EpsilonGreedyBanditArmInfo arm;
  arm.segment = *segment;
  arm.pulls = dict.FindInt(kPullsKey).value_or(0);
  arm.value = dict.FindDouble(kValueKey).value_or(1.0);

  return arm;
}

}  // namespace

base::Value::Dict EpsilonGreedyBanditArmsToValue(
    const EpsilonGreedyBanditArmMap& arms) {
  base::Value::Dict dict;

  for (const auto& [segment, arm] : arms) {
    dict.Set(segment, base::Value::Dict()
                          .Set(kSegmentKey, segment)
                          .Set(kPullsKey, arm.pulls)
                          .Set(kValueKey, arm.value));
  }

  return dict;
}

EpsilonGreedyBanditArmMap EpsilonGreedyBanditArmsFromValue(
    const base::Value::Dict& dict) {
  bool found_errors = false;

  EpsilonGreedyBanditArmMap arms;

  for (const auto [segment, value] : dict) {
    const auto* const item_dict = value.GetIfDict();
    if (!item_dict) {
      found_errors = true;
      continue;
    }

    const absl::optional<EpsilonGreedyBanditArmInfo> arm =
        MaybeGetArmFromDict(*item_dict);
    if (!arm) {
      found_errors = true;
      continue;
    }

    arms[segment] = *arm;
  }

  if (found_errors) {
    BLOG(0, "Error parsing epsilon greedy bandit arms");
  }

  return arms;
}

}  // namespace brave_ads
