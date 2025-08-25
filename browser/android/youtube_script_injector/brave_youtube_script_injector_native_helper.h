/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_
#define BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_

#include "content/public/browser/web_contents.h"

namespace youtube_script_injector {

// Enters Picture-in-Picture mode for the given WebContents
void EnterPictureInPicture(content::WebContents* web_contents);

}  // namespace youtube_script_injector

#endif  // BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_BRAVE_YOUTUBE_SCRIPT_INJECTOR_NATIVE_HELPER_H_
