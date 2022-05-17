/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/permission_rule_util.h"

#include <string>

#include "base/check.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/serving/permission_rules/permission_rule_interface.h"

namespace ads {

bool ShouldAllow(PermissionRuleInterface* permission_rule) {
  DCHECK(permission_rule);

  if (permission_rule->ShouldAllow()) {
    return true;
  }

  const std::string& last_message = permission_rule->GetLastMessage();
  if (!last_message.empty()) {
    BLOG(2, last_message);
  }

  return false;
}

}  // namespace ads
