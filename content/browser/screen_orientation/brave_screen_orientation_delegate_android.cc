/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/screen_orientation/brave_screen_orientation_delegate_android.h"
#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"
#include "content/browser/screen_orientation/screen_orientation_delegate_android.h"

#include "content/browser/screen_orientation/screen_orientation_provider.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "ui/base/device_form_factor.h"


namespace content {
BraveScreenOrientationDelegateAndroid::BraveScreenOrientationDelegateAndroid() = default;
BraveScreenOrientationDelegateAndroid::~BraveScreenOrientationDelegateAndroid() = default;

void BraveScreenOrientationDelegateAndroid::Lock(
    WebContents* web_contents,
    device::mojom::ScreenOrientationLockType lock_orientation) {
  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents);
  if (helper && helper->HasFullscreenBeenRequested()) {
    return;
  }

  ScreenOrientationDelegateAndroid::Lock(web_contents, lock_orientation);
}

void BraveScreenOrientationDelegateAndroid::Unlock(WebContents* web_contents) {
  YouTubeScriptInjectorTabHelper* helper =
      YouTubeScriptInjectorTabHelper::FromWebContents(web_contents);
  if (helper && helper->HasFullscreenBeenRequested()) {
    return;
  }

  ScreenOrientationDelegateAndroid::Unlock(web_contents);
}

} // namespace content
