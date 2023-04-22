/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "base/check.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/platform/platform_helper.h"

namespace brave_ads {

namespace {

constexpr int kDoNotDisturbFromHour = 21;  // 9pm
constexpr int kDoNotDisturbToHour = 6;     // 6am

bool DoesRespectCap() {
  if (PlatformHelper::GetInstance().GetType() != PlatformType::kAndroid) {
    return true;
  }

  if (BrowserManager::GetInstance()->IsBrowserActive()) {
    return true;
  }

  const base::Time time = base::Time::Now();
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return exploded.hour >= kDoNotDisturbToHour &&
         exploded.hour < kDoNotDisturbFromHour;
}

}  // namespace

base::expected<void, std::string> DoNotDisturbPermissionRule::ShouldAllow()
    const {
  if (!DoesRespectCap()) {
    return base::unexpected("Should not disturb");
  }

  return base::ok();
}

}  // namespace brave_ads
