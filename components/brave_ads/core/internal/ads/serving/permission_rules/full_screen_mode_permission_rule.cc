/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/full_screen_mode_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  if (!kShouldOnlyServeAdsInWindowedMode.Get()) {
    return true;
  }

  if (PlatformHelper::GetInstance().IsMobile()) {
    return true;
  }

  return !AdsClientHelper::GetInstance()->IsBrowserInFullScreenMode();
}

}  // namespace

base::expected<void, std::string> FullScreenModePermissionRule::ShouldAllow()
    const {
  if (!DoesRespectCap()) {
    return base::unexpected("Full screen mode");
  }

  return base::ok();
}

}  // namespace brave_ads
