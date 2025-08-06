/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/screen_orientation/brave_screen_orientation_delegate_android.h"

#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"

namespace content {

// Define the key constant
const char kYouTubeFullscreenPageDataKey[] = "youtube_fullscreen_page_data";

BraveScreenOrientationDelegateAndroid::BraveScreenOrientationDelegateAndroid() =
    default;
BraveScreenOrientationDelegateAndroid::
    ~BraveScreenOrientationDelegateAndroid() = default;

void BraveScreenOrientationDelegateAndroid::Lock(
    WebContents* web_contents,
    device::mojom::ScreenOrientationLockType lock_orientation) {
  // Check YouTube fullscreen state from NavigationEntry UserData
  if (web_contents) {
    NavigationEntry* entry =
        web_contents->GetController().GetLastCommittedEntry();
    if (entry) {
      auto* data = static_cast<YouTubeFullscreenPageData*>(
          entry->GetUserData(kYouTubeFullscreenPageDataKey));
      if (data && data->fullscreen_requested()) {
        return;
      }
    }
  }

  ScreenOrientationDelegateAndroid::Lock(web_contents, lock_orientation);
}

void BraveScreenOrientationDelegateAndroid::Unlock(WebContents* web_contents) {
  // Check YouTube fullscreen state from NavigationEntry UserData
  if (web_contents) {
    NavigationEntry* entry =
        web_contents->GetController().GetLastCommittedEntry();
    if (entry) {
      auto* data = static_cast<YouTubeFullscreenPageData*>(
          entry->GetUserData(kYouTubeFullscreenPageDataKey));
      if (data && data->fullscreen_requested()) {
        return;
      }
    }
  }

  ScreenOrientationDelegateAndroid::Unlock(web_contents);
}

}  // namespace content
