/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

#include "bat/ads/internal/platform/platform_helper.h"
#include "bat/ads/internal/user_activity/user_activity_scoring_util.h"

namespace ads {

UserActivityFrequencyCap::UserActivityFrequencyCap() = default;

UserActivityFrequencyCap::~UserActivityFrequencyCap() = default;

bool UserActivityFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "User was inactive";
    return false;
  }

  return true;
}

std::string UserActivityFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool UserActivityFrequencyCap::DoesRespectCap() {
  if (PlatformHelper::GetInstance()->GetPlatform() == PlatformType::kIOS) {
    return true;
  }

  return WasUserActive();
}

}  // namespace ads
