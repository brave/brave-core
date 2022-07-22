/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notification_helper/notification_helper.h"

#include "base/memory/singleton.h"
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl.h"
#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl_android.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl_linux.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl_mac.h"
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_ads/notification_helper/notification_helper_impl_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace brave_ads {

NotificationHelper::NotificationHelper() {
#if BUILDFLAG(IS_ANDROID)
  impl_.reset(new NotificationHelperImplAndroid());
#elif BUILDFLAG(IS_LINUX)
  impl_.reset(new NotificationHelperImplLinux());
#elif BUILDFLAG(IS_MAC)
  impl_.reset(new NotificationHelperImplMac());
#elif BUILDFLAG(IS_WIN)
  impl_.reset(new NotificationHelperImplWin());
#else
  // Default notification helper for unsupported platforms
  impl_.reset(new NotificationHelperImpl());
#endif  // BUILDFLAG(IS_ANDROID)
}

NotificationHelper::~NotificationHelper() = default;

// static
NotificationHelper* NotificationHelper::GetInstance() {
  return base::Singleton<NotificationHelper>::get();
}

bool NotificationHelper::CanShowNativeNotifications() {
  return impl_->CanShowNativeNotifications();
}

bool NotificationHelper::CanShowNativeNotificationsWhileBrowserIsBackgrounded()
    const {
  return impl_->CanShowNativeNotificationsWhileBrowserIsBackgrounded();
}

bool NotificationHelper::ShowOnboardingNotification() {
  return impl_->ShowOnboardingNotification();
}

}  // namespace brave_ads
