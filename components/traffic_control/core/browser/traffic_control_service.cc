// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_service.h"

namespace traffic_control {

TrafficControlService::TrafficControlService(PrefService* prefs)
    : prefs_(prefs) {
  CHECK(prefs_);
}

TrafficControlService::~TrafficControlService() = default;

bool TrafficControlService::IsEnabled() const {
  return prefs_->GetBoolean(prefs::kTrafficControlEnabled);
}

std::vector<mojom::TrafficRulePtr> TrafficControlService::GetRules() const {
  return GetRulesFromPrefs(*prefs_);
}

}  // namespace traffic_control
