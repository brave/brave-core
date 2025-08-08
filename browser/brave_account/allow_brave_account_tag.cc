/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_account/allow_brave_account_tag.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/android/chrome_jni_headers/BraveAccountAllowTag_jni.h"
#include "content/public/browser/web_contents.h"
#endif

void AllowBraveAccountTag::Mark(content::WebContents* web_contents) {
  AllowBraveAccountTag::CreateForWebContents(web_contents);
}

bool AllowBraveAccountTag::IsSet(content::WebContents* web_contents) {
  return AllowBraveAccountTag::FromWebContents(web_contents);
}

AllowBraveAccountTag::~AllowBraveAccountTag() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(AllowBraveAccountTag);

#if BUILDFLAG(IS_ANDROID)
namespace brave_account {

// static
void JNI_BraveAccountAllowTag_Mark(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  if (auto* web_contents =
          content::WebContents::FromJavaWebContents(jweb_contents)) {
    AllowBraveAccountTag::Mark(web_contents);
  }
}

}  // namespace brave_account
#endif
