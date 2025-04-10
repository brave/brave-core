/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/background_video/background_video_playback_tab_helper.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/android/background_video/features.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace {
constexpr char16_t kYoutubeBackgroundPlaybackAndPipScript[] =
    uR"(
(function() {
    if (document._addEventListener === undefined) {
        document._addEventListener = document.addEventListener;
        document.addEventListener = function(a,b,c) {
            if(a != 'visibilitychange') {
                document._addEventListener(a,b,c);
            }
        };
    }
}());
// Function to modify the flags if the target object exists.
function modifyYtcfgFlags() {
  if (!window.ytcfg) {
    return;
  }
  const config = window.ytcfg.get("WEB_PLAYER_CONTEXT_CONFIGS")?.WEB_PLAYER_CONTEXT_CONFIG_ID_MWEB_WATCH
  if (config && config.serializedExperimentFlags) {
    let flags = config.serializedExperimentFlags;

    // Replace target flags.
    flags = flags
      .replace("html5_picture_in_picture_blocking_ontimeupdate=true", "html5_picture_in_picture_blocking_ontimeupdate=false")
      .replace("html5_picture_in_picture_blocking_onresize=true", "html5_picture_in_picture_blocking_onresize=false")
      .replace("html5_picture_in_picture_blocking_document_fullscreen=true", "html5_picture_in_picture_blocking_document_fullscreen=false")
      .replace("html5_picture_in_picture_blocking_standard_api=true", "html5_picture_in_picture_blocking_standard_api=false")
      .replace("html5_picture_in_picture_logging_onresize=true", "html5_picture_in_picture_logging_onresize=false");

    // Assign updated flags back to config.
    config.serializedExperimentFlags = flags;
    if (observer) {
      observer.disconnect();
    }
  }
}
const observer = new MutationObserver((mutations) => {
  for (const mutation of mutations) {
    if (mutation.type === "childList" && mutation.addedNodes.length > 0) {
      mutation.addedNodes.forEach((node) => {
        if (node.tagName === "SCRIPT") {
          // Check and modify flags when a new script is added.
          modifyYtcfgFlags();
        }
      });
    }
  }
});
observer.observe(document.documentElement, { childList: true, subtree: true });
)";

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

  if (!base::FeatureList::IsEnabled(
          ::preferences::features::kBraveBackgroundVideoPlayback) &&
      !prefs->GetBoolean(kBackgroundVideoPlaybackEnabled)) {
    return false;
  }

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
  if (!IsYouTubeDomain(web_contents()->GetLastCommittedURL())) {
    return;
  }
  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    web_contents()->GetPrimaryMainFrame()->ExecuteJavaScript(
        kYoutubeBackgroundPlaybackAndPipScript, base::NullCallback());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper);
