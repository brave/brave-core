// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/android/rewards/rewards_page_helper.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/RewardsPageActivity_jni.h"

namespace brave_rewards {

void OpenURL(const std::string& url) {
  Java_RewardsPageActivity_openURL(
      base::android::AttachCurrentThread(),
      base::android::ConvertUTF8ToJavaString(
          base::android::AttachCurrentThread(), url));
}

}  // namespace brave_rewards
