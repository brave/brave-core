/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notification_helper/notification_helper.h"

#include "base/memory/singleton.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper_holder.h"
#include "build/build_config.h"

namespace brave_ads {

NotificationHelper::NotificationHelper() = default;

NotificationHelper::~NotificationHelper() = default;

bool NotificationHelper::CanShowNativeNotifications() {
  return true;
}

bool NotificationHelper::CanShowNativeNotificationsWhileBrowserIsBackgrounded()
    const {
  return true;
}

bool NotificationHelper::ShowOnboardingNotification() {
  return false;
}

// static
NotificationHelper* NotificationHelper::GetInstance() {
  NotificationHelperHolder* holder = NotificationHelperHolder::GetInstance();
  return holder->GetNotificationHelper();
}

}  // namespace brave_ads
