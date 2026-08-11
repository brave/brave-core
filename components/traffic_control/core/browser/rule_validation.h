// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_RULE_VALIDATION_H_
#define BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_RULE_VALIDATION_H_

#include <optional>
#include <string_view>

#include "brave/components/traffic_control/core/mojom/traffic_control.mojom-forward.h"

namespace traffic_control {

// Returns true if `filter` is a valid policy URL-blocklist filter string
// (see url_matcher::util::FilterToComponents).
bool IsValidUrlFilter(std::string_view filter);

// Returns an error if the given rule properties are invalid for add/update.
std::optional<mojom::RuleOperationError> ValidateRule(
    const mojom::TrafficRulePtr& rule,
    bool require_empty_id);

}  // namespace traffic_control

#endif  // BRAVE_COMPONENTS_TRAFFIC_CONTROL_CORE_BROWSER_RULE_VALIDATION_H_
