// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include <algorithm>
#include <utility>

#include "base/uuid.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/browser/url_filter_validation.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_service.h"

namespace traffic_control {

TrafficControlService::TrafficControlService(PrefService* prefs)
    : prefs_(prefs) {
  CHECK(prefs_);
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kTrafficControlList,
      base::BindRepeating(&TrafficControlService::OnRulesPrefChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kTrafficControlEnabled,
      base::BindRepeating(&TrafficControlService::OnEnabledPrefChanged,
                          base::Unretained(this)));
}

TrafficControlService::~TrafficControlService() = default;

void TrafficControlService::AddObserver(
    TrafficControlServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void TrafficControlService::RemoveObserver(
    TrafficControlServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool TrafficControlService::IsEnabled() const {
  return prefs_->GetBoolean(prefs::kTrafficControlEnabled);
}

void TrafficControlService::SetEnabled(bool enabled) {
  prefs_->SetBoolean(prefs::kTrafficControlEnabled, enabled);
}

std::vector<mojom::TrafficRulePtr> TrafficControlService::GetRules() const {
  return GetRulesFromPrefs(*prefs_);
}

std::optional<mojom::RuleOperationError> TrafficControlService::AddRule(
    mojom::TrafficRulePtr rule) {
  if (auto error = ValidateRule(rule, /*require_empty_id=*/true)) {
    return error;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  rule->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  rules.push_back(std::move(rule));
  SetRulesToPrefs(std::move(rules), *prefs_);
  return std::nullopt;
}

std::optional<mojom::RuleOperationError> TrafficControlService::UpdateRule(
    mojom::TrafficRulePtr rule) {
  if (auto error = ValidateRule(rule, /*require_empty_id=*/false)) {
    return error;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  auto it = std::ranges::find(rules, rule->id, &mojom::TrafficRule::id);
  if (it == rules.end()) {
    return mojom::RuleOperationError::kNotFound;
  }
  *it = std::move(rule);
  SetRulesToPrefs(std::move(rules), *prefs_);
  return std::nullopt;
}

std::optional<mojom::RuleOperationError> TrafficControlService::RemoveRule(
    std::string_view id) {
  if (id.empty()) {
    return mojom::RuleOperationError::kIdShouldBeSet;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  auto it = std::ranges::find(rules, id, &mojom::TrafficRule::id);
  if (it == rules.end()) {
    return mojom::RuleOperationError::kNotFound;
  }
  rules.erase(it);
  SetRulesToPrefs(std::move(rules), *prefs_);
  return std::nullopt;
}

void TrafficControlService::OnRulesPrefChanged() {
  for (auto& observer : observers_) {
    observer.OnRulesChanged();
  }
}

void TrafficControlService::OnEnabledPrefChanged() {
  for (auto& observer : observers_) {
    observer.OnEnabledChanged();
  }
}

// static
std::optional<mojom::RuleOperationError> TrafficControlService::ValidateRule(
    const mojom::TrafficRulePtr& rule,
    bool require_empty_id) {
  if (!rule || !rule->target) {
    return mojom::RuleOperationError::kInvalidTarget;
  }
  if (require_empty_id) {
    if (!rule->id.empty()) {
      return mojom::RuleOperationError::kIdShouldBeEmpty;
    }
  } else if (rule->id.empty()) {
    return mojom::RuleOperationError::kIdShouldBeSet;
  }

  if (!IsValidUrlFilter(rule->url_filter)) {
    return mojom::RuleOperationError::kInvalidUrlFilter;
  }

  return std::nullopt;
}

}  // namespace traffic_control
