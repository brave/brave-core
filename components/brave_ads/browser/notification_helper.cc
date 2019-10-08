/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper.h"

namespace brave_ads {

NotificationHelper::NotificationHelper() = default;

NotificationHelper::~NotificationHelper() = default;

bool NotificationHelper::ShouldShowNotifications() const {
  return true;
}

bool NotificationHelper::ShowMyFirstAdNotification() const {
  return false;
}

bool NotificationHelper::CanShowBackgroundNotifications() const {
  return true;
}

#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_LINUX) && !defined(OS_ANDROID)  // NOLINT
NotificationHelper* NotificationHelper::GetInstance() {
  // just return a dummy notification helper for all other platforms
  return base::Singleton<NotificationHelper>::get();
}
#endif

}  // namespace brave_ads
