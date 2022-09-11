/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_COOKIE_CONSENT_NOTICES_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_COOKIE_CONSENT_NOTICES_H_

#include <jni.h>
#include <string>
#include "base/android/scoped_java_ref.h"

namespace chrome {
namespace android {

class BraveCookieConsentNotices {
 public:
  BraveCookieConsentNotices(JNIEnv* env,
                            const base::android::JavaRef<jobject>& obj);
  ~BraveCookieConsentNotices();

  void EnableFilter(JNIEnv* env);

  bool IsFilterListAvailable(JNIEnv* env);

  void Destroy(JNIEnv* env);

 private:
  base::android::ScopedJavaGlobalRef<jobject> jobj_;
};

}  // namespace android
}  // namespace chrome

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_COOKIE_CONSENT_NOTICES_H_
