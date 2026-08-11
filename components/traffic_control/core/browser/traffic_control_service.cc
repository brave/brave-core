// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/browser/traffic_control_service_observer.h"
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

void TrafficControlService::Shutdown() {
  pref_change_registrar_.RemoveAll();
}

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

}  // namespace traffic_control
