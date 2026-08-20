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

constexpr char kIdKey[] = "id";
constexpr char kEnabledKey[] = "enabled";
constexpr char kConditionKey[] = "condition";
constexpr char kUrlFilterKey[] = "url_filter";
constexpr char kTargetKey[] = "target";
constexpr char kContainerIdKey[] = "container_id";

mojom::TrafficRulePtr RuleFromDict(const base::DictValue& dict) {
  const std::string* id = dict.FindString(kIdKey);
  std::optional<bool> enabled = dict.FindBool(kEnabledKey);
  const base::DictValue* condition = dict.FindDict(kConditionKey);
  const base::DictValue* target = dict.FindDict(kTargetKey);
  if (!id || !enabled.has_value() || !condition || !target) {
    LOG(ERROR) << "Traffic rule is missing required fields";
    return nullptr;
  }

  const std::string* url_filter = condition->FindString(kUrlFilterKey);
  if (!url_filter) {
    LOG(ERROR) << "Traffic rule condition is missing url_filter";
    return nullptr;
  }

  std::optional<std::string> container_id;
  if (const std::string* stored_container_id =
          target->FindString(kContainerIdKey)) {
    if (!stored_container_id->empty()) {
      container_id = *stored_container_id;
    }
  } else if (target->contains(kContainerIdKey)) {
    LOG(ERROR) << "Traffic rule target has a non-string container_id";
    return nullptr;
  }

  return mojom::TrafficRule::New(*id, *enabled, *url_filter,
                                 mojom::Target::New(std::move(container_id)));
}

base::DictValue RuleToDict(const mojom::TrafficRulePtr& rule) {
  base::DictValue condition;
  condition.Set(kUrlFilterKey, rule->url_filter);

  base::DictValue target;
  if (rule->target->container_id.has_value() &&
      !rule->target->container_id->empty()) {
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
