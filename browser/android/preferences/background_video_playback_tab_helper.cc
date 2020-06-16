/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
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

const char k_youtube_ads_block_script[] =
    "(function() {"
    "    const prunePaths = ['playerResponse.adPlacements',"
    "        'playerResponse.playerAds', 'adPlacements', 'playerAds'];"
    "    const findOwner = function(root, path) {"
    "        let owner = root;"
    "        let chain = path;"
    "        for (;;) {"
    "            if ( owner instanceof Object === false ) { return; }"
    "            const pos = chain.indexOf('.');"
    "            if ( pos === -1 ) {"
    "                return owner.hasOwnProperty(chain)? [ owner, chain ]:"
    "                    undefined;"
    "            }"
    "            const prop = chain.slice(0, pos);"
    "            if ( owner.hasOwnProperty(prop) === false ) { return; }"
    "            owner = owner[prop];"
    "            chain = chain.slice(pos + 1);"
    "        }"
    "    };"
    "    JSON.parse = new Proxy(JSON.parse, {"
    "        apply: function() {"
    "            const r = Reflect.apply(...arguments);"
    "            for ( const path of prunePaths ) {"
    "                const details = findOwner(r, path);"
    "                if ( details !== undefined ) {"
    "                    delete details[0][details[1]];"
    "                }"
    "            }"
    "            return r;"
    "        },"
    "    });"
    "})();";

bool IsYouTubeDomain(content::WebContents* contents) {
  if (net::registry_controlled_domains::SameDomainOrHost(
          contents->GetLastCommittedURL(), GURL("https://www.youtube.com"),
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

bool IsAdBlockEnabled(content::WebContents* contents) {
  Profile* profile = static_cast<Profile*>(contents->GetBrowserContext());

  brave_shields::ControlType control_type =
      brave_shields::GetAdControlType(profile,
      contents->GetLastCommittedURL());
  if (brave_shields::GetBraveShieldsEnabled(profile,
        contents->GetLastCommittedURL()) &&
      control_type != brave_shields::ALLOW) {
    content::RenderFrameHost::AllowInjectingJavaScript();

    return true;
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
  // Filter only YT domains here
  if (!IsYouTubeDomain(web_contents())) {
    return;
  }
  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    web_contents()->GetMainFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(k_youtube_background_playback_script),
        base::NullCallback());
  }
  if (IsAdBlockEnabled(web_contents())) {
    web_contents()->GetMainFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(k_youtube_ads_block_script),
        base::NullCallback());
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper)
