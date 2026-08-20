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
//       "target":    { "container_id": "" }             // optional fields
//     },
//     ...
//   ]
//
// Nested `condition` / `target` objects mirror the mojom structs so newer
// clients can add optional fields without breaking older ones (unknown keys are
// ignored on read). Unset optional fields are omitted; empty strings are
// preserved.
constexpr char kIdKey[] = "id";
constexpr char kEnabledKey[] = "enabled";
constexpr char kConditionKey[] = "condition";
constexpr char kUrlFilterKey[] = "url_filter";
constexpr char kTargetKey[] = "target";
constexpr char kContainerIdKey[] = "container_id";

// Deserializes an optional string field from a nested dict.
// Returns false when the key exists but is not a string (malformed sync data).
// Empty strings are preserved; only a missing key means unset.
bool ReadOptionalString(const base::DictValue& dict,
                        const char* key,
                        std::optional<std::string>& out) {
  if (const std::string* value = dict.FindString(key)) {
    out = *value;
    return true;
  }
  if (dict.contains(key)) {
    return false;
  }
  return true;
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
  std::optional<std::string> url_filter;
  if (!ReadOptionalString(*condition_dict, kUrlFilterKey, url_filter)) {
    LOG(ERROR) << "Traffic rule condition has a non-string url_filter";
    return nullptr;
  }

  // Target: optional fields (currently container_id). An empty string is
  // meaningful ("open outside a container") and must be preserved.
  std::optional<std::string> container_id;
  if (!ReadOptionalString(*target_dict, kContainerIdKey, container_id)) {
    LOG(ERROR) << "Traffic rule target has a non-string container_id";
    return nullptr;
  }

  return mojom::TrafficRule::New(*id, *enabled,
                                 mojom::Condition::New(std::move(url_filter)),
                                 mojom::Target::New(std::move(container_id)));
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
