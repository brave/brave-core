// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_H_

#include <vector>

#include "base/values.h"
#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"

class PrefService;

namespace traffic_control {

// Returns the list of traffic rules stored in preferences.
std::vector<mojom::TrafficRulePtr> GetRulesFromPrefs(const PrefService& prefs);

// Stores the provided list of traffic rules in preferences (full replace).
void SetRulesToPrefs(const std::vector<mojom::TrafficRulePtr>& rules,
                     PrefService& prefs);

// Converts a list of rules to a base::ListValue for pref storage.
base::ListValue ConvertRulesToListValue(
    const std::vector<mojom::TrafficRulePtr>& rules);

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_PREFS_H_
