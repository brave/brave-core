/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/user_activity_permission_rule.h"

#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/user_interaction/user_activity/user_activity_scoring_util.h"

namespace ads {

namespace {

bool DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  if (PlatformHelper::GetInstance()->GetType() == PlatformType::kIOS) {
    return true;
  }

  return WasUserActive();
}

}  // namespace

bool UserActivityPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "User was inactive";
    return false;
  }

  return true;
}

const std::string& UserActivityPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
