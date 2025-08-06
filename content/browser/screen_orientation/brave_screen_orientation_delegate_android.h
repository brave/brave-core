/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_SCREEN_ORIENTATION_BRAVE_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_
#define BRAVE_CONTENT_BROWSER_SCREEN_ORIENTATION_BRAVE_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_

#include "base/supports_user_data.h"
#include "content/browser/screen_orientation/screen_orientation_delegate_android.h"
#include "content/common/content_export.h"
#include "content/public/browser/screen_orientation_delegate.h"
#include "content/public/browser/web_contents.h"
#include "services/device/public/mojom/screen_orientation_lock_types.mojom-shared.h"

namespace content {

// Shared data structure for tracking YouTube fullscreen state
struct CONTENT_EXPORT YouTubeFullscreenPageData
    : public base::SupportsUserData::Data {
 public:
  explicit YouTubeFullscreenPageData(bool fullscreen_requested = false)
      : fullscreen_requested_(fullscreen_requested) {}

  bool fullscreen_requested() const { return fullscreen_requested_; }
  void set_fullscreen_requested(bool requested) {
    fullscreen_requested_ = requested;
  }

 private:
  bool fullscreen_requested_;
};

// Key for storing YouTubeFullscreenPageData in NavigationEntry UserData
CONTENT_EXPORT extern const char kYouTubeFullscreenPageDataKey[];

class CONTENT_EXPORT BraveScreenOrientationDelegateAndroid
    : public ScreenOrientationDelegateAndroid {
 public:
  BraveScreenOrientationDelegateAndroid();
  ~BraveScreenOrientationDelegateAndroid() override;

  BraveScreenOrientationDelegateAndroid(
      const BraveScreenOrientationDelegateAndroid&) = delete;
  BraveScreenOrientationDelegateAndroid& operator=(
      const BraveScreenOrientationDelegateAndroid&) = delete;

  void Lock(WebContents* web_contents,
            device::mojom::ScreenOrientationLockType lock_orientation) override;
  void Unlock(WebContents* web_contents) override;

 private:
  // Helper method to check if YouTube fullscreen is requested
  static bool IsYouTubeFullscreenRequested(WebContents* web_contents);
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_SCREEN_ORIENTATION_BRAVE_SCREEN_ORIENTATION_DELEGATE_ANDROID_H_
