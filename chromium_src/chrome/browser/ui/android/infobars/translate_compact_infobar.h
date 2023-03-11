/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_INFOBARS_TRANSLATE_COMPACT_INFOBAR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_INFOBARS_TRANSLATE_COMPACT_INFOBAR_H_

// Add a method that will be used to redirect chromium impl in .cc file.
#define action_flags_                \
  action_flags_;                     \
  jboolean IsIncognito_ChromiumImpl( \
      JNIEnv* env, const base::android::JavaParamRef<jobject>& obj)
#include "src/chrome/browser/ui/android/infobars/translate_compact_infobar.h"  // IWYU pragma: export
#undef action_flags_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_ANDROID_INFOBARS_TRANSLATE_COMPACT_INFOBAR_H_
