/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_targeting/models/bandits/epsilon_greedy_bandit_arms.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_targeting {

namespace {
const char kArmSegmentKey[] = "segment";
const char kArmValueKey[] = "value";
const char kArmPullsKey[] = "pulls";
}  // namespace

EpsilonGreedyBanditArms::EpsilonGreedyBanditArms() = default;

EpsilonGreedyBanditArms::~EpsilonGreedyBanditArms() = default;

EpsilonGreedyBanditArmList EpsilonGreedyBanditArms::FromJson(
    const std::string& json) {
  EpsilonGreedyBanditArmList arms;
  base::Optional<base::Value> value = base::JSONReader::Read(json);
  if (!value || !value->is_list()) {
    return arms;
  }

  base::ListValue* list = nullptr;
  if (!value->GetAsList(&list)) {
    return arms;
  }

  arms = GetFromList(list);
  return arms;
}

std::string EpsilonGreedyBanditArms::ToJson(
    const EpsilonGreedyBanditArmList& arms) {
  base::Value list(base::Value::Type::LIST);

  for (const auto& arm : arms) {
    base::Value dictionary(base::Value::Type::DICTIONARY);
    dictionary.SetKey(kArmSegmentKey, base::Value(arm.segment));
    dictionary.SetKey(kArmPullsKey,
        base::Value(std::to_string(arm.pulls)));
    dictionary.SetKey(kArmValueKey,
        base::Value(std::to_string(arm.value)));
    list.Append(std::move(dictionary));
  }

  std::string json;
  base::JSONWriter::Write(list, &json);

  return json;
}

///////////////////////////////////////////////////////////////////////////////

EpsilonGreedyBanditArmList EpsilonGreedyBanditArms::GetFromList(
    const base::ListValue* list) {
  EpsilonGreedyBanditArmList arms;

  DCHECK(list);
  if (!list) {
    return arms;
  }

  for (const auto& value : list->GetList()) {
    if (!value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* dictionary = nullptr;
    value.GetAsDictionary(&dictionary);
    if (!dictionary) {
      NOTREACHED();
      continue;
    }

    EpsilonGreedyBanditArmInfo arm;
    if (!GetFromDictionary(dictionary, &arm)) {
      NOTREACHED();
      continue;
    }

    arms.push_back(arm);
  }

  return arms;
}

bool EpsilonGreedyBanditArms::GetFromDictionary(
    const base::DictionaryValue* dictionary,
    EpsilonGreedyBanditArmInfo* info) {
  DCHECK(dictionary);
  if (!dictionary) {
    return false;
  }

  DCHECK(info);
  if (!info) {
    return false;
  }

  EpsilonGreedyBanditArmInfo arm;

  // Segment
  const auto* segment = dictionary->FindStringKey(kArmSegmentKey);
  if (!segment) {
    return false;
  }
  arm.segment = *segment;

  // Pulls
  const auto* pulls = dictionary->FindStringKey(kArmPullsKey);
  if (!pulls) {
    return false;
  }
  if (!base::StringToUint64(*pulls, &arm.pulls)) {
    return false;
  }

  // Value
  const auto* value =
      dictionary->FindStringKey(kArmValueKey);
  if (!value) {
    return false;
  }

  if (!base::StringToDouble(*value, &arm.value)) {
    return false;
  }

  *info = arm;
  return true;
}

}  // namespace ad_targeting
}  // namespace ads
