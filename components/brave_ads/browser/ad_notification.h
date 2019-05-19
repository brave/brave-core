/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_

#include "base/android/scoped_java_ref.h"
#include "net/base/completion_once_callback.h"
#include <jni.h>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "bat/ads/notification_info.h"
#include "build/build_config.h"
#include "base/android/scoped_java_ref.h"

namespace message_center {
class Notification;
}

namespace brave_ads {

std::unique_ptr<message_center::Notification> CreateAdNotification(
      const ads::NotificationInfo& notification_info,
      std::string* notification_id);
/*
class BraveAds {
  public:
    static void OnShowHelper(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_profile_android, jstring uuid);
    static void OnClickHelper(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_profile_android, jstring url, bool should_close);
    static void OnDismissHelper(JNIEnv* env, const base::android::JavaParamRef<jobject>& j_profile_android, jstring url, jstring uuid, bool dismissed_by_user);
};
*/


}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_AD_NOTIFICATION_
