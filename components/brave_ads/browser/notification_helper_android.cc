/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_ads/browser/notification_helper_android.h"
#include "chrome/android/chrome_jni_headers/NotificationSystemStatusUtil_jni.h"
#include "chrome/android/chrome_jni_headers/BraveAds_jni.h"
#include "chrome/android/chrome_jni_headers/BraveAdsSignupDialog_jni.h"
#include "base/android/jni_string.h"
#include "base/system/sys_info.h"

namespace brave_ads {

namespace {

const int kMinimumVersionForNotificationChannels = 8;

const int kAppNotificationStatusUndeterminable = 0;
const int kAppNotificationsStatusEnabled = 2;

}

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::ShouldShowNotifications() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  int status = Java_NotificationSystemStatusUtil_getAppNotificationStatus(env);
  bool is_notifications_enabled = (status == kAppNotificationsStatusEnabled ||
      status == kAppNotificationStatusUndeterminable);

  bool is_notification_channel_enabled = IsBraveAdsNotificationChannelEnabled();

  return is_notifications_enabled && is_notification_channel_enabled;
}

bool NotificationHelperAndroid::ShowMyFirstAdNotification() const {
  if (!ShouldShowNotifications()) {
    return false;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAdsSignupDialog_enqueueOobeNotificationNative(env);

  return true;
}

bool NotificationHelperAndroid::IsBraveAdsNotificationChannelEnabled() const {
  if (GetOperatingSystemVersion() < kMinimumVersionForNotificationChannels) {
    return true;
  }

  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_channel_id = Java_BraveAds_getBraveAdsChannelId(env);
  std::string channel_id = ConvertJavaStringToUTF8(env, j_channel_id);

  auto status = channels_provider_->GetChannelStatus(channel_id);
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

NotificationHelperAndroid* NotificationHelperAndroid::GetInstance() {
  return base::Singleton<NotificationHelperAndroid>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperAndroid::GetInstance();
}

}  // namespace brave_ads
