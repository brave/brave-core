/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notification_helper/notification_helper_holder.h"

#include "base/memory/singleton.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_ads/notification_helper/notification_helper_android.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/brave_ads/notification_helper/notification_helper_linux.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_ads/notification_helper/notification_helper_mac.h"
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_ads/notification_helper/notification_helper_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace brave_ads {

NotificationHelperHolder::NotificationHelperHolder() {
#if BUILDFLAG(IS_ANDROID)
  notification_helper_.reset(new NotificationHelperAndroid());
#elif BUILDFLAG(IS_LINUX)
  notification_helper_.reset(new NotificationHelperLinux());
#elif BUILDFLAG(IS_MAC)
  notification_helper_.reset(new NotificationHelperMac());
#elif BUILDFLAG(IS_WIN)
  notification_helper_.reset(new NotificationHelperWin());
#else
  // Default notification helper for unsupported platforms
  notification_helper_.reset(new NotificationHelper());
#endif  // BUILDFLAG(IS_ANDROID)
}

NotificationHelperHolder::~NotificationHelperHolder() = default;

// static
NotificationHelperHolder* NotificationHelperHolder::GetInstance() {
  return base::Singleton<NotificationHelperHolder>::get();
}

NotificationHelper* NotificationHelperHolder::GetNotificationHelper() {
  return notification_helper_.get();
}

}  // namespace brave_ads
