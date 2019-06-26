/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_android.h"
#include "jni/NotificationSystemStatusUtil_jni.h"
#include "jni/BraveAds_jni.h"
#include "base/android/jni_string.h"
#include "base/system/sys_info.h"

namespace brave_ads {

const int kMaxAndroidChannelVersion = 7;

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::IsNotificationsAvailable() const {
  const int kAppNotificationStatusUndeterminable = 0;
  const int kAppNotificationsStatusEnabled = 2;
  int status = Java_NotificationSystemStatusUtil_getAppNotificationStatus(
      base::android::AttachCurrentThread());
  bool notificationsOn = (status == kAppNotificationsStatusEnabled ||
      status == kAppNotificationStatusUndeterminable);
  bool channelOn = IsBraveAdsChannelEnabled();
  return (notificationsOn && channelOn);
}

// Starting in Android 8.0 (API level 26), all notifications must be
// assigned to a channel.
bool NotificationHelperAndroid::IsBraveAdsChannelEnabled() const {
  if (NotificationHelperAndroid::GetOsVersion() <= kMaxAndroidChannelVersion) {
    return true;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_id = Java_BraveAds_getBraveAdsChannelId (env);
  std::string channel_id = base::android::ConvertJavaStringToUTF8(env, j_id);
  auto status = channels_provider_->GetChannelStatus(channel_id);
  return (NotificationChannelStatus::ENABLED == status ||
      NotificationChannelStatus::UNAVAILABLE == status);
}

int NotificationHelperAndroid::GetOsVersion() const {
  int32_t os_major_version = 0;
  int32_t os_minor_version = 0;
  int32_t os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(
      &os_major_version, &os_minor_version, &os_bugfix_version);
  return os_major_version;
}

NotificationHelperAndroid* NotificationHelperAndroid::GetInstance() {
  return base::Singleton<NotificationHelperAndroid>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperAndroid::GetInstance();
}

}  // namespace brave_ads
