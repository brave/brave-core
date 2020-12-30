/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification_helper_android.h"

#include <string>

#include "base/android/jni_string.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsSignupDialog_jni.h"

namespace brave_ads {

NotificationHelperAndroid::NotificationHelperAndroid() = default;

NotificationHelperAndroid::~NotificationHelperAndroid() = default;

bool NotificationHelperAndroid::ShouldShowNotifications() {
  return true;
}

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

}  // namespace brave_ads
