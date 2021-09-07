/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/do_not_disturb_frequency_cap.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/platform/platform_helper.h"

namespace ads {

namespace {

const int kDoNotDisturbFromHour = 21;  // 9pm
const int kDoNotDisturbToHour = 6;     // 6am

}  // namespace

DoNotDisturbFrequencyCap::DoNotDisturbFrequencyCap() = default;

DoNotDisturbFrequencyCap::~DoNotDisturbFrequencyCap() = default;

bool DoNotDisturbFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Should not disturb";
    return false;
  }

  return true;
}

std::string DoNotDisturbFrequencyCap::get_last_message() const {
  return last_message_;
}

bool DoNotDisturbFrequencyCap::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetPlatform() != PlatformType::kAndroid) {
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
