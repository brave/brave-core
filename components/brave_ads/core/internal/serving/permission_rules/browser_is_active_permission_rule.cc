/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/browser_is_active_permission_rule.h"

#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  if (!kShouldOnlyServeAdsIfBrowserIsActive.Get()) {
    return true;
  }

  return BrowserManager::GetInstance().IsActive() &&
         BrowserManager::GetInstance().IsInForeground();
}

}  // namespace

base::expected<void, std::string> BrowserIsActivePermissionRule::ShouldAllow()
    const {
  if (!DoesRespectCap()) {
    return base::unexpected("Browser window is not active");
  }

  return base::ok();
}

}  // namespace brave_ads
