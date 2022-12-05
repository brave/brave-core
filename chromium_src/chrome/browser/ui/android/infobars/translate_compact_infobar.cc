/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/android/infobars/translate_compact_infobar.h"

#include "brave/components/translate/core/common/brave_translate_features.h"
#include "chrome/android/chrome_jni_headers/TranslateCompactInfoBar_jni.h"
#include "components/translate/core/browser/translate_driver.h"

#define IsIncognito IsIncognito_ChromiumImpl
#include "src/chrome/browser/ui/android/infobars/translate_compact_infobar.cc"
#undef IsIncognito

jboolean TranslateCompactInfoBar::IsIncognito(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& obj) {
  if (!translate::IsBraveAutoTranslateEnabled()) {
    // Setting incognito mode disables UI elements related to auto
    // translate. "Never translate <something>" options should still work.
    return true;
  }

  return IsIncognito_ChromiumImpl(env, obj);
}
