/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/trace_event/trace_event.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_android.h"
#endif  // BUILDFLAG(IS_ANDROID)

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_linux.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_mac.h"
#endif  // BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_WIN)
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_win.h"
#include "chrome/browser/notifications/notification_platform_bridge_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace {

bool SystemNotificationsEnabled(Profile* profile) {
#if BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
  return true;
#elif BUILDFLAG(IS_WIN)
  return NotificationPlatformBridgeWin::SystemNotificationEnabled();
#else
#if BUILDFLAG(IS_LINUX)
  if (profile) {
    // Prefs take precedence over flags.
    PrefService* prefs = profile->GetPrefs();
    if (!prefs->GetBoolean(prefs::kAllowSystemNotifications)) {
      return false;
    }
  }
#endif  // BUILDFLAG(IS_LINUX)
  return base::FeatureList::IsEnabled(features::kNativeNotifications) &&
         base::FeatureList::IsEnabled(features::kSystemNotifications);
#endif  // BUILDFLAG(IS_CHROMEOS) || BUILDFLAG(IS_ANDROID)
}

NotificationPlatformBridge* GetSystemNotificationPlatformBridge(
    Profile* profile) {
  if (SystemNotificationsEnabled(profile)) {
    return g_browser_process->notification_platform_bridge();
  }

  // The platform does not support, or has not enabled, system notifications.
  return nullptr;
}

}  // namespace

namespace brave_ads {

NotificationHelper::NotificationHelper() {
  TRACE_EVENT("brave.ads", "NotificationHelper::NotificationHelper");
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
  static base::NoDestructor<NotificationHelper> instance;
  return instance.get();
}

void NotificationHelper::MaybeInitForProfile(Profile* profile,
                                             base::OnceClosure callback) {
  if (is_initialized_) {
    return std::move(callback).Run();
  }
  is_initialized_ = true;

  NotificationPlatformBridge* system_bridge =
      GetSystemNotificationPlatformBridge(profile);
  if (!system_bridge) {
    does_support_system_notifications_ = false;
    return std::move(callback).Run();
  }

  system_bridge->SetReadyCallback(base::BindOnce(
      &NotificationHelper::OnSystemNotificationPlatformBridgeReady,
      weak_factory_.GetWeakPtr(), std::move(callback)));
}

bool NotificationHelper::CanShowNotifications() {
  TRACE_EVENT("brave.ads", "NotificationHelper::CanShowNotifications");
  return impl_->CanShowNotifications();
}

bool NotificationHelper::CanShowSystemNotificationsWhileBrowserIsBackgrounded()
    const {
  TRACE_EVENT("brave.ads",
              "NotificationHelper::"
              "CanShowSystemNotificationsWhileBrowserIsBackgrounded");
  if (!does_support_system_notifications_) {
    return false;
  }
  return impl_->CanShowSystemNotificationsWhileBrowserIsBackgrounded();
}

bool NotificationHelper::ShowOnboardingNotification() {
  TRACE_EVENT("brave.ads", "NotificationHelper::ShowOnboardingNotification");
  return impl_->ShowOnboardingNotification();
}

bool NotificationHelper::DoesSupportSystemNotifications() const {
  return does_support_system_notifications_;
}

void NotificationHelper::OnSystemNotificationPlatformBridgeReady(
    base::OnceClosure callback,
    bool success) {
  does_support_system_notifications_ = success;

  impl_->InitSystemNotifications(std::move(callback));
}

}  // namespace brave_ads
