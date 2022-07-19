/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/behavioral/bandits/epsilon_greedy_bandit_arms.h"

#include <utility>

#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/base/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace targeting {

namespace {

constexpr char kSegmentKey[] = "segment";
constexpr char kValueKey[] = "value";
constexpr char kPullsKey[] = "pulls";

bool GetArmFromDictionary(const base::Value::Dict& dict,
                          EpsilonGreedyBanditArmInfo* info) {
  DCHECK(info);

  EpsilonGreedyBanditArmInfo arm;
  const std::string* segment = dict.FindString(kSegmentKey);
  if (!segment || segment->empty()) {
    return false;
  }
  arm.segment = *segment;
  arm.pulls = dict.FindInt(kPullsKey).value_or(0);
  arm.value = dict.FindDouble(kValueKey).value_or(1.0);
  *info = arm;
  return true;
}

EpsilonGreedyBanditArmMap GetArmsFromDictionary(const base::Value::Dict& dict) {
  EpsilonGreedyBanditArmMap arms;
  bool found_errors = false;
  for (const auto [key, value] : dict) {
    if (!value.is_dict()) {
      found_errors = true;
      continue;
    }
    const base::Value::Dict& arm_dict = value.GetDict();

    EpsilonGreedyBanditArmInfo arm;
    if (!GetArmFromDictionary(arm_dict, &arm)) {
      found_errors = true;
      continue;
    }

    arms[key] = arm;
  }

  if (found_errors) {
    BLOG(0, "Errors detected when parsing epsilon greedy bandit arms");
  }
  return arms;
}

}  // namespace

EpsilonGreedyBanditArms::EpsilonGreedyBanditArms() = default;

EpsilonGreedyBanditArms::~EpsilonGreedyBanditArms() = default;

EpsilonGreedyBanditArmMap EpsilonGreedyBanditArms::FromJson(
    const std::string& json) {
  EpsilonGreedyBanditArmMap arms;
  absl::optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_dict()) {
    return arms;
  }
  const base::Value::Dict& arm_dict = value->GetDict();

  arms = GetArmsFromDictionary(arm_dict);
  return arms;
}

std::string EpsilonGreedyBanditArms::ToJson(
    const EpsilonGreedyBanditArmMap& arms) {
  base::Value::Dict dict;

  for (const auto& [key, value] : arms) {
    base::Value::Dict item;
    item.Set(kSegmentKey, key);
    item.Set(kPullsKey, value.pulls);
    item.Set(kValueKey, value.value);
    dict.Set(key, std::move(item));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace targeting
}  // namespace ads
