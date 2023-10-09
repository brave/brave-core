/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/user_activity_permission_rule_unittest_util.h"

#include "brave/components/brave_ads/core/internal/user/user_attention/user_activity/user_activity_manager.h"

namespace brave_ads::test {

void ForceUserActivityPermissionRule() {
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kOpenedNewTab);
  UserActivityManager::GetInstance().RecordEvent(
      UserActivityEventType::kClosedTab);
}

}  // namespace brave_ads::test
