/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/data_types/behavioral/bandits/epsilon_greedy_bandit_arms.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {

namespace {

const char kSegmentKey[] = "segment";
const char kValueKey[] = "value";
const char kPullsKey[] = "pulls";

bool GetArmFromDictionary(const base::DictionaryValue* dictionary,
                          EpsilonGreedyBanditArmInfo* info) {
  DCHECK(dictionary);
  DCHECK(info);

  if (!dictionary) {
    return false;
  }

  if (!info) {
    return false;
  }

  EpsilonGreedyBanditArmInfo arm;

  const std::string* segment = dictionary->FindStringKey(kSegmentKey);
  if (!segment) {
    return false;
  }
  arm.segment = *segment;

  arm.pulls = dictionary->FindIntKey(kPullsKey).value_or(0);

  arm.value = dictionary->FindDoubleKey(kValueKey).value_or(1.0);

  *info = arm;

  return true;
}

EpsilonGreedyBanditArmMap GetArmsFromDictionary(
    const base::DictionaryValue* dictionary) {
  DCHECK(dictionary);
  EpsilonGreedyBanditArmMap arms;

  if (!dictionary) {
    return arms;
  }

  for (const auto& value : dictionary->DictItems()) {
    if (!value.second.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* arm_dictionary = nullptr;
    value.second.GetAsDictionary(&arm_dictionary);
    if (!arm_dictionary) {
      NOTREACHED();
      continue;
    }

    EpsilonGreedyBanditArmInfo arm;
    if (!GetArmFromDictionary(arm_dictionary, &arm)) {
      NOTREACHED();
      continue;
    }

    arms[value.first] = arm;
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

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return arms;
  }

  arms = GetArmsFromDictionary(dictionary);
  return arms;
}

std::string EpsilonGreedyBanditArms::ToJson(
    const EpsilonGreedyBanditArmMap& arms) {
  base::Value arms_dictionary(base::Value::Type::DICTIONARY);

  for (const auto& arm : arms) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetKey(kSegmentKey, base::Value(arm.first));
    dictionary.SetKey(kPullsKey, base::Value(arm.second.pulls));
    dictionary.SetKey(kValueKey, base::Value(arm.second.value));
    arms_dictionary.SetKey(arm.first, std::move(dictionary));
  }

  std::string json;
  base::JSONWriter::Write(arms_dictionary, &json);

  return json;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace ad_targeting
}  // namespace ads
