/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_android.h"
#include "jni/NotificationSystemStatusUtil_jni.h"
#include "jni/BraveAds_jni.h"
#include "base/android/jni_string.h"

namespace brave_ads {

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::IsNotificationsAvailable() const {
  bool notificationsOn = IsBraveNotificationsEnabled();
  bool channelOn = IsBraveAdsChannelEnabled();
  return (notificationsOn && channelOn);
}

void NotificationHelperAndroid::OpenNotificationsSettings() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAds_openSystemNotifPrefs(env);
}

// Starting in Android 8.0 (API level 26), all notifications must be
// assigned to a channel.
bool NotificationHelperAndroid::IsBraveAdsChannelEnabled() const {
  JNIEnv* env = base::android::AttachCurrentThread();
  auto j_id = Java_BraveAds_getBraveAdsChannelId (env);
  std::string channel_id = base::android::ConvertJavaStringToUTF8(env, j_id);
  auto status = channels_provider_->GetChannelStatus(channel_id);
  return (NotificationChannelStatus::ENABLED == status ||
      NotificationChannelStatus::UNAVAILABLE == status);
}

bool NotificationHelperAndroid::IsBraveNotificationsEnabled() const {
  const int kAppNotificationStatusUndeterminable = 0;
  const int kAppNotificationsStatusEnabled = 2;
  int status = Java_NotificationSystemStatusUtil_getAppNotificationStatus(
      base::android::AttachCurrentThread());
  return (status == kAppNotificationsStatusEnabled ||
      status == kAppNotificationStatusUndeterminable);
}

NotificationHelperAndroid* NotificationHelperAndroid::GetInstance() {
  return base::Singleton<NotificationHelperAndroid>::get();
}

NotificationHelper* NotificationHelper::GetInstance() {
  return NotificationHelperAndroid::GetInstance();
}

}  // namespace brave_ads
