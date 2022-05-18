/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/internal/base/platform_helper.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"

namespace ads {

namespace {

constexpr int kDoNotDisturbFromHour = 21;  // 9pm
constexpr int kDoNotDisturbToHour = 6;     // 6am

}  // namespace

DoNotDisturbPermissionRule::DoNotDisturbPermissionRule() = default;

DoNotDisturbPermissionRule::~DoNotDisturbPermissionRule() = default;

bool DoNotDisturbPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Should not disturb";
    return false;
  }

  return true;
}

std::string DoNotDisturbPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool DoNotDisturbPermissionRule::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetType() != PlatformType::kAndroid) {
    return true;
  }

  if (BrowserManager::Get()->IsActive()) {
    return true;
  }

  const base::Time time = base::Time::Now();
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  if (exploded.hour >= kDoNotDisturbToHour &&
      exploded.hour < kDoNotDisturbFromHour) {
    return true;
  }

  return false;
}

}  // namespace ads
