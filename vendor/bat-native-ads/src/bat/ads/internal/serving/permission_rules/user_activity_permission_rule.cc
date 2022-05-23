/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/user_interaction/browsing/user_activity_scoring_util.h"

namespace ads {

UserActivityPermissionRule::UserActivityPermissionRule() = default;

UserActivityPermissionRule::~UserActivityPermissionRule() = default;

bool UserActivityPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "User was inactive";
    return false;
  }

  return true;
}

std::string UserActivityPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool UserActivityPermissionRule::DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  if (PlatformHelper::GetInstance()->GetType() == PlatformType::kIOS) {
    return true;
  }

  return WasUserActive();
}

}  // namespace ads
