/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_android.h"

#include <string>

#include "base/android/jni_string.h"
#include "base/system/sys_info.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsSignupDialog_jni.h"
#include "brave/components/brave_ads/browser/background_helper.h"
#include "chrome/browser/notifications/notification_channels_provider_android.h"

namespace brave_ads {

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::ShowMyFirstAdNotification() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_BraveAdsSignupDialog_enqueueOnboardingNotificationNative(env);

  return true;
}

NotificationHelperAndroid* NotificationHelperAndroid::GetInstanceImpl() {
  return base::Singleton<NotificationHelperAndroid>::get();
}

NotificationHelper* NotificationHelper::GetInstanceImpl() {
  return NotificationHelperAndroid::GetInstanceImpl();
}

///////////////////////////////////////////////////////////////////////////////

int NotificationHelperAndroid::GetOperatingSystemVersion() const {
  int32_t major_version = 0;
  int32_t minor_version = 0;
  int32_t bugfix_version = 0;

  base::SysInfo::OperatingSystemVersionNumbers(
      &major_version, &minor_version, &bugfix_version);

  return major_version;
}

}  // namespace brave_ads
