/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
#define BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class YouTubeScriptInjectorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<YouTubeScriptInjectorTabHelper> {
 public:
  explicit YouTubeScriptInjectorTabHelper(content::WebContents* contents);
  YouTubeScriptInjectorTabHelper(const YouTubeScriptInjectorTabHelper&) =
      delete;
  YouTubeScriptInjectorTabHelper& operator=(
      const YouTubeScriptInjectorTabHelper&) = delete;
  ~YouTubeScriptInjectorTabHelper() override;
  static bool IsYouTubeVideo(const GURL& url);

  // content::WebContentsObserver overrides:
  void PrimaryMainDocumentElementAvailable() override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_ANDROID_YOUTUBE_SCRIPT_INJECTOR_YOUTUBE_SCRIPT_INJECTOR_TAB_HELPER_H_
