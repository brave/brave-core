/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

namespace {
const char k_youtube_background_playback_script[] =
    "(function() {"
    "    if (document._addEventListener === undefined) {"
    "        document._addEventListener = document.addEventListener;"
    "        document.addEventListener = function(a,b,c) {"
    "            if(a != 'visibilitychange') {"
    "                document._addEventListener(a,b,c);"
    "            }"
    "        };"
    "    }"
    "}());";

bool IsBackgroundVideoPlaybackEnabled(content::WebContents* contents) {
  PrefService* prefs =
      static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();
  if (!prefs->GetBoolean(kBackgroundVideoPlaybackEnabled))
    return false;

  const std::string host = contents->GetLastCommittedURL().host();
  const std::string path = contents->GetLastCommittedURL().path();
  if (path.find("/watch") != std::string::npos) {
    if (host.find("www.youtube.com") != std::string::npos ||
        host.find("youtube.com") != std::string::npos ||
        host.find("m.youtube.com") != std::string::npos) {
        return true;
    }
  }

  return false;
}

}  // namespace

BackgroundVideoPlaybackTabHelper::BackgroundVideoPlaybackTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents) {
}

BackgroundVideoPlaybackTabHelper::~BackgroundVideoPlaybackTabHelper() {
}

void BackgroundVideoPlaybackTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    web_contents()->GetMainFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(k_youtube_background_playback_script),
        base::NullCallback());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper)
