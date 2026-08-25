// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/prefs.h"

#include <optional>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/types/expected.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_service.h"

namespace traffic_control {
namespace {

// Pref storage for traffic control rules is a list of rule dicts:
//
//   [
//     {
//       "id": "...",  // uuid
//       "enabled": true,
//       "condition": { "url_filter": "example.com" },   // optional fields
//       "target": {                                     // optional fields
//         "container_id": "",
//         "temporary_container": true
//       }
//     },
//     ...
//   ]
//
// Nested `condition` / `target` objects mirror the mojom structs so newer
// clients can add optional fields without breaking older ones (unknown keys are
// ignored on read). Unset optional fields are omitted; empty strings are
// preserved. `temporary_container` is omitted when false.
constexpr char kIdKey[] = "id";
constexpr char kEnabledKey[] = "enabled";
constexpr char kConditionKey[] = "condition";
constexpr char kUrlFilterKey[] = "url_filter";
constexpr char kTargetKey[] = "target";
constexpr char kContainerIdKey[] = "container_id";
constexpr char kTemporaryContainerKey[] = "temporary_container";

enum class PrefFieldError {
  kWrongType,
};

// Deserializes an optional string field from a nested dict.
// Empty strings are preserved; only a missing key means unset.
// Wrong type (malformed sync data) => PrefFieldError::kWrongType.
base::expected<std::optional<std::string>, PrefFieldError> ReadOptionalString(
    const base::DictValue& dict,
    std::string_view key) {
  if (const std::string* value = dict.FindString(key)) {
    return *value;
  }
  if (dict.contains(key)) {
    return base::unexpected(PrefFieldError::kWrongType);
  }
  return std::nullopt;
}

// Deserializes an optional bool. Missing key => false.
// Wrong type => PrefFieldError::kWrongType.
base::expected<bool, PrefFieldError> ReadOptionalBool(
    const base::DictValue& dict,
    std::string_view key) {
  if (std::optional<bool> value = dict.FindBool(key)) {
    return *value;
  }
  if (dict.contains(key)) {
    return base::unexpected(PrefFieldError::kWrongType);
  }
  return false;
}

mojom::TrafficRulePtr RuleFromDict(const base::DictValue& dict) {
  const std::string* id = dict.FindString(kIdKey);
  std::optional<bool> enabled = dict.FindBool(kEnabledKey);
  const base::DictValue* condition_dict = dict.FindDict(kConditionKey);
  const base::DictValue* target_dict = dict.FindDict(kTargetKey);
  if (!id || !enabled.has_value() || !condition_dict || !target_dict) {
    LOG(ERROR) << "Traffic rule is missing required fields";
    return nullptr;
  }

  // Condition: optional fields (currently url_filter). Absent keys mean unset;
  // a present non-string value is treated as corrupt and the rule is skipped.
  auto url_filter = ReadOptionalString(*condition_dict, kUrlFilterKey);
  if (!url_filter.has_value()) {
    LOG(ERROR) << "Traffic rule condition has a non-string url_filter";
    return nullptr;
  }

  // Target: optional fields. An empty container_id string is meaningful
  // ("open outside a container") and must be preserved.
  auto container_id = ReadOptionalString(*target_dict, kContainerIdKey);
  if (!container_id.has_value()) {
    LOG(ERROR) << "Traffic rule target has a non-string container_id";
    return nullptr;
  }
  auto temporary_container =
      ReadOptionalBool(*target_dict, kTemporaryContainerKey);
  if (!temporary_container.has_value()) {
    LOG(ERROR) << "Traffic rule target has a non-bool temporary_container";
    return nullptr;
  }

  return mojom::TrafficRule::New(
      *id, *enabled, mojom::Condition::New(std::move(*url_filter)),
      mojom::Target::New(std::move(*container_id), *temporary_container));
}

base::DictValue RuleToDict(const mojom::TrafficRulePtr& rule) {
  base::DictValue condition;
  if (rule->condition->url_filter.has_value()) {
    condition.Set(kUrlFilterKey, *rule->condition->url_filter);
  }

  base::DictValue target;
  if (rule->target->container_id.has_value()) {
    target.Set(kContainerIdKey, *rule->target->container_id);
  }
  if (rule->target->temporary_container) {
    target.Set(kTemporaryContainerKey, true);
  }

  return base::DictValue()
      .Set(kIdKey, rule->id)
      .Set(kEnabledKey, rule->enabled)
      .Set(kConditionKey, std::move(condition))
      .Set(kTargetKey, std::move(target));
}

}  // namespace

std::vector<mojom::TrafficRulePtr> GetRulesFromPrefs(const PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kTrafficControl));
  std::vector<mojom::TrafficRulePtr> rules;
  for (const auto& entry : prefs.GetList(prefs::kTrafficControlList)) {
    if (!entry.is_dict()) {
      LOG(ERROR) << "Traffic rule is not a dictionary";
      continue;
    }
    if (auto parsed = RuleFromDict(entry.GetDict())) {
      rules.push_back(std::move(parsed));
    }
  }
  return rules;
}

void SetRulesToPrefs(const std::vector<mojom::TrafficRulePtr>& rules,
                     PrefService& prefs) {
  CHECK(base::FeatureList::IsEnabled(features::kTrafficControl));
  prefs.SetList(prefs::kTrafficControlList, ConvertRulesToListValue(rules));
}

base::ListValue ConvertRulesToListValue(
    const std::vector<mojom::TrafficRulePtr>& rules) {
  base::ListValue list;
  for (const auto& rule : rules) {
    CHECK(rule);
    list.Append(RuleToDict(rule));
  }
  return list;
}

}  // namespace traffic_control
