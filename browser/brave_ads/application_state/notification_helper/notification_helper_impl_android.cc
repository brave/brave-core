/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper_impl_android.h"

#include "base/android/jni_string.h"
#include "base/system/sys_info.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsSignupDialog_jni.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAds_jni.h"
#include "brave/build/android/jni_headers/BraveSiteChannelsManagerBridge_jni.h"
#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "brave/components/brave_ads/browser/application_state/background_helper.h"
#include "chrome/browser/notifications/jni_headers/NotificationSystemStatusUtil_jni.h"
#include "chrome/browser/notifications/notification_channels_provider_android.h"

namespace brave_ads {

namespace {

constexpr int kMinimumMajorOperatingSystemVersionForNotificationChannels = 8;

constexpr int kAppNotificationStatusUndeterminable = 0;
constexpr int kAppNotificationsStatusEnabled = 2;

int GetOperatingSystemMajorVersion() {
  int32_t major_version = 0;
  int32_t minor_version = 0;
  int32_t bugfix_version = 0;

  base::SysInfo::OperatingSystemVersionNumbers(&major_version, &minor_version,
                                               &bugfix_version);

  return major_version;
}

bool IsBraveAdsNotificationChannelEnabled(bool is_foreground) {
  if (GetOperatingSystemMajorVersion() <
      kMinimumMajorOperatingSystemVersionForNotificationChannels) {
    return true;
  }

  JNIEnv* env = jni_zero::AttachCurrentThread();

  const auto j_channel_id =
      (is_foreground) ? Java_BraveAds_getBraveAdsChannelId(env)
                      : Java_BraveAds_getBraveAdsBackgroundChannelId(env);

  const auto status = static_cast<NotificationChannelStatus>(
      Java_BraveSiteChannelsManagerBridge_getChannelStatus(env, j_channel_id));

  return (status == NotificationChannelStatus::ENABLED ||
          status == NotificationChannelStatus::UNAVAILABLE);
}

}  // namespace

NotificationHelperImplAndroid::NotificationHelperImplAndroid() = default;

NotificationHelperImplAndroid::~NotificationHelperImplAndroid() = default;

bool NotificationHelperImplAndroid::CanShowNotifications() {
  JNIEnv* env = jni_zero::AttachCurrentThread();
  const int status =
      Java_NotificationSystemStatusUtil_getAppNotificationStatus(env);

  const bool is_notifications_enabled =
      (status == kAppNotificationsStatusEnabled ||
       status == kAppNotificationStatusUndeterminable);

  const bool is_foreground = BackgroundHelper::GetInstance()->IsForeground();

  const bool is_notification_channel_enabled =
      IsBraveAdsNotificationChannelEnabled(is_foreground);

  bool can_show_native_notifications =
      is_notifications_enabled && is_notification_channel_enabled;

  if (!is_foreground) {
    can_show_native_notifications =
        can_show_native_notifications &&
        CanShowSystemNotificationsWhileBrowserIsBackgrounded();
  }

  return can_show_native_notifications;
}

bool NotificationHelperImplAndroid::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() const {
  JNIEnv* env = jni_zero::AttachCurrentThread();
  return Java_BraveAdsSignupDialog_showAdsInBackground(env);
}

bool NotificationHelperImplAndroid::ShowOnboardingNotification() {
  const bool should_show_custom_notifications =
      base::FeatureList::IsEnabled(kCustomNotificationAdFeature);

  JNIEnv* env = jni_zero::AttachCurrentThread();
  Java_BraveAdsSignupDialog_enqueueOnboardingNotificationNative(
      env, should_show_custom_notifications);

  return true;
}

}  // namespace brave_ads
