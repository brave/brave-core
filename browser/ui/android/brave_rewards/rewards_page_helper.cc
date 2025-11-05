// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/android/brave_rewards/rewards_page_helper.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
// Suppress unused function warning for JNI functions we don't use
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#include "brave/build/android/jni_headers/TabUtils_jni.h"
#pragma clang diagnostic pop

namespace brave_rewards {

void OpenURLForRewardsPage(const std::string& url) {
  Java_TabUtils_openURLWithBraveActivity(
      base::android::AttachCurrentThread(),
      base::android::ConvertUTF8ToJavaString(
          base::android::AttachCurrentThread(), url));
}

}  // namespace brave_rewards
