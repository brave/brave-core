// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include "base/functional/bind.h"
#include "base/types/optional_ref.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace traffic_control {

TrafficControlService::TrafficControlService(PrefService* prefs)
    : prefs_(prefs) {
  CHECK(prefs_);
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      prefs::kTrafficControlList,
      base::BindRepeating(&TrafficControlService::RebuildMatcher,
                          base::Unretained(this)));
  RebuildMatcher();
}

TrafficControlService::~TrafficControlService() = default;

bool TrafficControlService::IsEnabled() const {
  return prefs_->GetBoolean(prefs::kTrafficControlEnabled);
}

std::vector<mojom::TrafficRulePtr> TrafficControlService::GetRules() const {
  return GetRulesFromPrefs(*prefs_);
}

base::optional_ref<const mojom::TrafficRule>
TrafficControlService::FindMatchingRule(const GURL& url) const {
  if (!IsEnabled()) {
    return std::nullopt;
  }
  return matcher_.FindMatchingRule(url);
}

void TrafficControlService::Shutdown() {
  pref_change_registrar_.RemoveAll();
}

void TrafficControlService::RebuildMatcher() {
  matcher_.Rebuild(GetRulesFromPrefs(*prefs_));
}

}  // namespace traffic_control
