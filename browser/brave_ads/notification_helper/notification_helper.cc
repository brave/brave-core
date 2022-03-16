/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notification_helper/notification_helper.h"

#include "build/build_config.h"

namespace brave_ads {

NotificationHelper* g_notification_helper_for_testing = nullptr;

NotificationHelper::NotificationHelper() = default;

NotificationHelper::~NotificationHelper() = default;

void NotificationHelper::set_for_testing(
    NotificationHelper* notification_helper) {
  g_notification_helper_for_testing = notification_helper;
}

bool NotificationHelper::CanShowNativeNotifications() {
  return true;
}

bool NotificationHelper::CanShowBackgroundNotifications() const {
  return true;
}

bool NotificationHelper::ShowMyFirstAdNotification() {
  return false;
}

NotificationHelper* NotificationHelper::GetInstance() {
  if (g_notification_helper_for_testing) {
    return g_notification_helper_for_testing;
  }

  return GetInstanceImpl();
}

#if !BUILDFLAG(IS_MAC) && !BUILDFLAG(IS_WIN) && !BUILDFLAG(IS_LINUX) && \
    !BUILDFLAG(IS_ANDROID)
NotificationHelper* NotificationHelper::GetInstanceImpl() {
  // Return a default notification helper for unsupported platforms
  return base::Singleton<NotificationHelper>::get();
}
#endif

}  // namespace brave_ads
