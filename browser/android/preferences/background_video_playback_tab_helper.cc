/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace {
const char16_t k_youtube_background_playback_script[] =
    u"(function() {"
    "    if (document._addEventListener === undefined) {"
    "        document._addEventListener = document.addEventListener;"
    "        document.addEventListener = function(a,b,c) {"
    "            if(a != 'visibilitychange') {"
    "                document._addEventListener(a,b,c);"
    "            }"
    "        };"
    "    }"
    "}());";

bool IsYouTubeDomain(const GURL& url) {
  if (net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://www.youtube.com"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return true;
  }

  return false;
}

bool IsBackgroundVideoPlaybackEnabled(content::WebContents* contents) {
  PrefService* prefs =
      static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();
  if (!prefs->GetBoolean(kBackgroundVideoPlaybackEnabled))
    return false;

  content::RenderFrameHost::AllowInjectingJavaScript();

  return true;
}
}  // namespace

BackgroundVideoPlaybackTabHelper::BackgroundVideoPlaybackTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<BackgroundVideoPlaybackTabHelper>(
          *contents) {}

BackgroundVideoPlaybackTabHelper::~BackgroundVideoPlaybackTabHelper() {}

void BackgroundVideoPlaybackTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Filter only YT domain here
  if (!IsYouTubeDomain(GetWebContents().GetLastCommittedURL())) {
    return;
  }
  if (IsBackgroundVideoPlaybackEnabled(&GetWebContents())) {
    GetWebContents().GetMainFrame()->ExecuteJavaScript(
        k_youtube_background_playback_script, base::NullCallback());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper);
