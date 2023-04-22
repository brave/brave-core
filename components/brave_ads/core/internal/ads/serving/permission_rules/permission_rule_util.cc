/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_util.h"

#include <string>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_interface.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

bool ShouldAllow(PermissionRuleInterface* permission_rule) {
  DCHECK(permission_rule);

  const auto result = permission_rule->ShouldAllow();
  if (!result.has_value()) {
    BLOG(2, result.error());
    return false;
  }

  return true;
}

}  // namespace brave_ads
