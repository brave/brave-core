/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_linux.h"

#include "brave/components/brave_ads/browser/features.h"

namespace brave_ads {

NotificationHelperLinux::NotificationHelperLinux() = default;

NotificationHelperLinux::~NotificationHelperLinux() = default;

bool NotificationHelperLinux::ShouldShowNotifications() {
  return features::ShouldShowCustomAdNotifications();
}

bool NotificationHelperLinux::CanShowNativeNotifications() {
  // TODO(https://github.com/brave/brave-browser/issues/5542): Investigate how
  // to detect if notifications are enabled within the Linux operating system

  return false;
}

bool NotificationHelperLinux::ShowMyFirstAdNotification() {
  return false;
}

bool NotificationHelperLinux::CanShowBackgroundNotifications() const {
  return true;
}

NotificationHelperLinux* NotificationHelperLinux::GetInstanceImpl() {
  return base::Singleton<NotificationHelperLinux>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperLinux::GetInstanceImpl();
}

}  // namespace brave_ads
