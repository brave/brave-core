/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/full_screen_mode_permission_rule.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/features/frequency_capping_features.h"

namespace ads {

FullScreenModePermissionRule::FullScreenModePermissionRule() = default;

FullScreenModePermissionRule::~FullScreenModePermissionRule() = default;

bool FullScreenModePermissionRule::ShouldAllow() {
  if (!features::frequency_capping::ShouldOnlyServeAdsInWindowedMode()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Full screen mode";
    return false;
  }

  return true;
}

std::string FullScreenModePermissionRule::GetLastMessage() const {
  return last_message_;
}

bool FullScreenModePermissionRule::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->IsMobile()) {
    return true;
  }

  return !AdsClientHelper::Get()->IsBrowserInFullScreenMode();
}

}  // namespace ads
