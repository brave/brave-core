/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_android.h"

#include <string>

#include "base/android/jni_string.h"
#include "base/system/sys_info.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAds_jni.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsSignupDialog_jni.h"
#include "brave/build/android/jni_headers/BraveNotificationSettingsBridge_jni.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "chrome/android/chrome_jni_headers/NotificationSystemStatusUtil_jni.h"
#include "chrome/browser/notifications/notification_channels_provider_android.h"

namespace brave_ads {

namespace {

const int kMinimumVersionForNotificationChannels = 8;

const int kAppNotificationStatusUndeterminable = 0;
const int kAppNotificationsStatusEnabled = 2;

}

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::ShouldShowNotifications() {
  JNIEnv* env = base::android::AttachCurrentThread();
  int status = Java_NotificationSystemStatusUtil_getAppNotificationStatus(env);
  bool is_notifications_enabled = (status == kAppNotificationsStatusEnabled ||
      status == kAppNotificationStatusUndeterminable);

  bool is_foreground = BackgroundHelper::GetInstance()->IsForeground();
  bool is_notification_channel_enabled =
      IsBraveAdsNotificationChannelEnabled(is_foreground);
  bool result = is_notifications_enabled && is_notification_channel_enabled;
  if (!is_foreground) {
    result = result && CanShowBackgroundNotifications();
  }
  return result;
}

bool NotificationHelperAndroid::ShowMyFirstAdNotification() {
  if (!ShouldShowNotifications()) {
    return false;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAdsSignupDialog_enqueueOnboardingNotificationNative(env);

  return true;
}

bool NotificationHelperAndroid::CanShowBackgroundNotifications() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_BraveAdsSignupDialog_showAdsInBackground(env);
}

NotificationHelperAndroid* NotificationHelperAndroid::GetInstanceImpl() {
  return base::Singleton<NotificationHelperAndroid>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperAndroid::GetInstanceImpl();
}

///////////////////////////////////////////////////////////////////////////////

bool NotificationHelperAndroid::IsBraveAdsNotificationChannelEnabled
    (bool foreground_channel) const {
  if (GetOperatingSystemVersion() < kMinimumVersionForNotificationChannels) {
    return true;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_channel_id = (foreground_channel) ?
      Java_BraveAds_getBraveAdsChannelId(env):
      Java_BraveAds_getBraveAdsBackgroundChannelId(env);
  auto status = static_cast<NotificationChannelStatus>(
      Java_BraveNotificationSettingsBridge_getChannelStatus(
          env, j_channel_id));

  return (status == NotificationChannelStatus::ENABLED ||
      status == NotificationChannelStatus::UNAVAILABLE);
}

int NotificationHelperAndroid::GetOperatingSystemVersion() const {
  int32_t major_version = 0;
  int32_t minor_version = 0;
  int32_t bugfix_version = 0;

  base::SysInfo::OperatingSystemVersionNumbers(
      &major_version, &minor_version, &bugfix_version);

  return major_version;
}

}  // namespace brave_ads
