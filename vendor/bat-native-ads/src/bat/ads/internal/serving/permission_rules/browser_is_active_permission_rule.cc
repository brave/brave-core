/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/browser_is_active_permission_rule.h"

#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/features/frequency_capping_features.h"

namespace ads {

BrowserIsActivePermissionRule::BrowserIsActivePermissionRule() = default;

BrowserIsActivePermissionRule::~BrowserIsActivePermissionRule() = default;

bool BrowserIsActivePermissionRule::ShouldAllow() {
  if (!features::frequency_capping::ShouldOnlyServeAdsIfBrowserIsActive()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Browser window is not active";
    return false;
  }

  return true;
}

std::string BrowserIsActivePermissionRule::GetLastMessage() const {
  return last_message_;
}

bool BrowserIsActivePermissionRule::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetType() == PlatformType::kAndroid) {
    return true;
  }

  return BrowserManager::Get()->IsActive();
}

}  // namespace ads
