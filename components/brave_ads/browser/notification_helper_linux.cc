/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_linux.h"

#include "base/logging.h"
#include "chrome/browser/fullscreen.h"

namespace brave_ads {

NotificationHelperLinux::NotificationHelperLinux() = default;

NotificationHelperLinux::~NotificationHelperLinux() = default;

bool NotificationHelperLinux::ShouldShowNotifications() {
  if (IsFullScreenMode()) {
    LOG(WARNING) << "Notification not made: Full screen mode";
    return false;
  }

  return true;
}

bool NotificationHelperLinux::ShowMyFirstAdNotification() {
  return false;
}

NotificationHelperLinux* NotificationHelperLinux::GetInstanceImpl() {
  return base::Singleton<NotificationHelperLinux>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperLinux::GetInstanceImpl();
}

}  // namespace brave_ads
