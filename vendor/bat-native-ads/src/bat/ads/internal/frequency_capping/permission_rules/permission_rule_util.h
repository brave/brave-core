/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PERMISSION_RULE_UTIL_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PERMISSION_RULE_UTIL_H_  // NOLINT

#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule.h"

namespace ads {

bool ShouldAllow(
    PermissionRule* permission_rule);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PERMISSION_RULE_UTIL_H_  // NOLINT
