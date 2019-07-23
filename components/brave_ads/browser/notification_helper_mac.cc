/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_mac.h"

namespace brave_ads {

NotificationHelperMac::NotificationHelperMac() = default;

NotificationHelperMac::~NotificationHelperMac() = default;

bool NotificationHelperMac::IsNotificationsAvailable() const {
  return true;
}

bool NotificationHelperMac::ShowMyFirstAdNotification() const {
  return false;
}

NotificationHelperMac* NotificationHelperMac::GetInstance() {
  return base::Singleton<NotificationHelperMac>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperMac::GetInstance();
}

}  // namespace brave_ads
