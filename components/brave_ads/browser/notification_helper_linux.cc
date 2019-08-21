/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_linux.h"

namespace brave_ads {

NotificationHelperLinux::NotificationHelperLinux() = default;

NotificationHelperLinux::~NotificationHelperLinux() = default;

bool NotificationHelperLinux::ShouldShowNotifications() const {
  // Unable to find a way of detecting if notifications are enabled within the
  // operating system so always return true
  return true;
}

bool NotificationHelperLinux::ShowMyFirstAdNotification() const {
  return false;
}

NotificationHelperLinux* NotificationHelperLinux::GetInstance() {
  return base::Singleton<NotificationHelperLinux>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperLinux::GetInstance();
}

}  // namespace brave_ads
