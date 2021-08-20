/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REFERRALS_ANDROID_BRAVE_REFERRER_H_
#define BRAVE_BROWSER_BRAVE_REFERRALS_ANDROID_BRAVE_REFERRER_H_

#include <jni.h>
#include <memory>
#include <string>
#include <vector>

#include "base/android/scoped_java_ref.h"
#include "brave/browser/brave_referrals/android_brave_referrer.h"
#include "net/base/completion_once_callback.h"

namespace android_brave_referrer {

using InitReferrerCallback = base::OnceCallback<void()>;

class BraveReferrer {
 public:
  BraveReferrer();
  ~BraveReferrer();

  BraveReferrer(const BraveReferrer&) = delete;
  BraveReferrer& operator=(const BraveReferrer&) = delete;

  void InitReferrer(InitReferrerCallback init_referrer_callback);
  void OnReferrerReady(JNIEnv* env);

 private:
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
  InitReferrerCallback init_referrer_callback_;
};

}  // namespace android_brave_referrer

#endif  // BRAVE_BROWSER_BRAVE_REFERRALS_ANDROID_BRAVE_REFERRER_H_
