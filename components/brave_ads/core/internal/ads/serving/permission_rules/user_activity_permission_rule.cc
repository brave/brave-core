/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/user_activity_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_scoring_util.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (PlatformHelper::GetInstance().GetType() == PlatformType::kIOS) {
    return true;
  }

  return WasUserActive();
}

}  // namespace

base::expected<void, std::string> UserActivityPermissionRule::ShouldAllow()
    const {
  if (!DoesRespectCap()) {
    return base::unexpected("User was inactive");
  }

  return base::ok();
}

}  // namespace brave_ads
