// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_settings_handler.h"

#include <algorithm>
#include <utility>

#include "base/uuid.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/browser/rule_validation.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_service.h"

namespace traffic_control {

TrafficControlSettingsHandler::TrafficControlSettingsHandler(PrefService* prefs)
    : prefs_(prefs) {
  CHECK(prefs_);
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kTrafficControlList,
      base::BindRepeating(&TrafficControlSettingsHandler::OnRulesPrefChanged,
                          base::Unretained(this)));
}

TrafficControlSettingsHandler::~TrafficControlSettingsHandler() = default;

void TrafficControlSettingsHandler::BindUI(
    mojo::PendingRemote<mojom::TrafficControlSettingsUI> ui) {
  DCHECK(!ui_);
  ui_.Bind(std::move(ui));
}

void TrafficControlSettingsHandler::GetRules(GetRulesCallback callback) {
  std::move(callback).Run(GetRulesFromPrefs(*prefs_));
}

void TrafficControlSettingsHandler::AddRule(mojom::TrafficRulePtr rule,
                                            AddRuleCallback callback) {
  if (auto error = ValidateRule(rule, /*require_empty_id=*/true)) {
    std::move(callback).Run(*error);
    return;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  rule->id = base::Uuid::GenerateRandomV4().AsLowercaseString();
  rules.push_back(std::move(rule));
  SetRulesToPrefs(std::move(rules), *prefs_);
  std::move(callback).Run(std::nullopt);
}

void TrafficControlSettingsHandler::UpdateRule(mojom::TrafficRulePtr rule,
                                               UpdateRuleCallback callback) {
  if (auto error = ValidateRule(rule, /*require_empty_id=*/false)) {
    std::move(callback).Run(*error);
    return;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  auto it = std::ranges::find(rules, rule->id, &mojom::TrafficRule::id);
  if (it == rules.end()) {
    std::move(callback).Run(mojom::RuleOperationError::kNotFound);
    return;
  }
  *it = std::move(rule);
  SetRulesToPrefs(std::move(rules), *prefs_);
  std::move(callback).Run(std::nullopt);
}

void TrafficControlSettingsHandler::RemoveRule(const std::string& id,
                                               RemoveRuleCallback callback) {
  if (id.empty()) {
    std::move(callback).Run(mojom::RuleOperationError::kIdShouldBeSet);
    return;
  }

  auto rules = GetRulesFromPrefs(*prefs_);
  auto it = std::ranges::find(rules, id, &mojom::TrafficRule::id);
  if (it == rules.end()) {
    std::move(callback).Run(mojom::RuleOperationError::kNotFound);
    return;
  }
  rules.erase(it);
  SetRulesToPrefs(std::move(rules), *prefs_);
  std::move(callback).Run(std::nullopt);
}

void TrafficControlSettingsHandler::OnRulesPrefChanged() {
  if (ui_) {
    ui_->OnRulesChanged(GetRulesFromPrefs(*prefs_));
  }
}

}  // namespace traffic_control
