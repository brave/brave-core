/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_feature.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

namespace brave_ads {

bool HasDoNotDisturbPermission() {
  if (PlatformHelper::GetInstance().GetType() != PlatformType::kAndroid) {
    return true;
  }

  if (BrowserManager::GetInstance().IsActive() &&
      BrowserManager::GetInstance().IsInForeground()) {
    return true;
  }

  base::Time::Exploded exploded;
  base::Time::Now().LocalExplode(&exploded);

  if (exploded.hour >= kDoNotDisturbToHour.Get() &&
      exploded.hour < kDoNotDisturbFromHour.Get()) {
    return true;
  }

  BLOG(2, "Should not disturb");
  return false;
}

}  // namespace brave_ads
