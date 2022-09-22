/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/browser_is_active_permission_rule.h"

#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/base/platform/platform_helper.h"
#include "bat/ads/internal/browser/browser_manager.h"

namespace ads {

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

bool BrowserIsActivePermissionRule::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetType() == PlatformType::kAndroid) {
    return true;
  }

  return BrowserManager::GetInstance()->IsBrowserActive();
}

}  // namespace ads
