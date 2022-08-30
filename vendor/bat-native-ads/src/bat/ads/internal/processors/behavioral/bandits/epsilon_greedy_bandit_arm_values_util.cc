/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arm_values_util.h"

#include <string>
#include <utility>

#include "bat/ads/internal/base/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace targeting {

namespace {

constexpr char kSegmentKey[] = "segment";
constexpr char kValueKey[] = "value";
constexpr char kPullsKey[] = "pulls";

absl::optional<EpsilonGreedyBanditArmInfo> MaybeGetArmFromDict(
    const base::Value::Dict& dict) {
  const std::string* segment = dict.FindString(kSegmentKey);
  if (!segment || segment->empty()) {
    return absl::nullopt;
  }

  EpsilonGreedyBanditArmInfo arm;
  arm.segment = *segment;
  arm.pulls = dict.FindInt(kPullsKey).value_or(0);
  arm.value = dict.FindDouble(kValueKey).value_or(1.0);

  return arm;
}

absl::optional<EpsilonGreedyBanditArmInfo> MaybeGetArmFromValue(
    const base::Value& value) {
  const base::Value::Dict* dict = value.GetIfDict();
  if (!dict) {
    return absl::nullopt;
  }

  return MaybeGetArmFromDict(*dict);
}

}  // namespace

base::Value::Dict EpsilonGreedyBanditArmsToValue(
    const EpsilonGreedyBanditArmMap& arms) {
  base::Value::Dict dict;

  for (const auto& [key, value] : arms) {
    base::Value::Dict item;
    item.Set(kSegmentKey, key);
    item.Set(kPullsKey, value.pulls);
    item.Set(kValueKey, value.value);

    dict.Set(key, std::move(item));
  }

  return dict;
}

EpsilonGreedyBanditArmMap EpsilonGreedyBanditArmsFromValue(
    const base::Value::Dict& dict) {
  bool found_errors = false;

  EpsilonGreedyBanditArmMap arms;

  for (const auto [key, value] : dict) {
    const absl::optional<EpsilonGreedyBanditArmInfo> arm =
        MaybeGetArmFromValue(value);
    if (!arm) {
      found_errors = true;
      continue;
    }

    arms[key] = *arm;
  }

  if (found_errors) {
    BLOG(0, "Error parsing epsilon greedy bandit arms");
  }

  return arms;
}

}  // namespace targeting
}  // namespace ads
