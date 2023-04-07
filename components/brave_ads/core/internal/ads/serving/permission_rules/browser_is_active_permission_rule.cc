/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/browser_is_active_permission_rule.h"

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetType() == PlatformType::kAndroid) {
    return true;
  }

  return BrowserManager::GetInstance()->IsBrowserActive();
}

}  // namespace

bool BrowserIsActivePermissionRule::ShouldAllow() {
  if (!permission_rules::features::ShouldOnlyServeAdsIfBrowserIsActive()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Browser window is not active";
    return false;
  }

  return true;
}

const std::string& BrowserIsActivePermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace brave_ads
