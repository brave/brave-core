/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/allow_notifications_frequency_cap.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

AllowNotificationsFrequencyCap::AllowNotificationsFrequencyCap() = default;

AllowNotificationsFrequencyCap::~AllowNotificationsFrequencyCap() = default;

bool AllowNotificationsFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Notifications not allowed";
    return false;
  }

  return true;
}

std::string AllowNotificationsFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool AllowNotificationsFrequencyCap::DoesRespectCap() {
  return AdsClientHelper::Get()->ShouldShowNotifications();
}

}  // namespace ads
