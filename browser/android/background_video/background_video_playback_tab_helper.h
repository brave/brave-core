/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_BACKGROUND_VIDEO_BACKGROUND_VIDEO_PLAYBACK_TAB_HELPER_H_
#define BRAVE_BROWSER_ANDROID_BACKGROUND_VIDEO_BACKGROUND_VIDEO_PLAYBACK_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class BackgroundVideoPlaybackTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<BackgroundVideoPlaybackTabHelper> {
 public:
  explicit BackgroundVideoPlaybackTabHelper(content::WebContents* contents);
  BackgroundVideoPlaybackTabHelper(const BackgroundVideoPlaybackTabHelper&) =
      delete;
  BackgroundVideoPlaybackTabHelper& operator=(
      const BackgroundVideoPlaybackTabHelper&) = delete;
  ~BackgroundVideoPlaybackTabHelper() override;

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_ANDROID_BACKGROUND_VIDEO_BACKGROUND_VIDEO_PLAYBACK_TAB_HELPER_H_
