/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_
#define BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_

#include "base/android/scoped_java_ref.h"
#include "url/gurl.h"

namespace youtube_script_injector {

static jboolean JNI_BraveYouTubeScriptInjectorNativeHelper_IsYouTubeVideo(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents);

static void JNI_BraveYouTubeScriptInjectorNativeHelper_SetFullscreen(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents);

}  // namespace youtube_script_injector

#endif  // BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_
