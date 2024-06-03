/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/preferences/background_video_playback_tab_helper.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/android/preferences/features.h"
#include "brave/build/android/jni_headers/BackgroundVideoPlaybackTabHelper_jni.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "content/browser/media/session/media_session_impl.h"
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
    "           if(a != 'visibilitychange') {"
    "               document._addEventListener(a,b,c);"
    "           }"
    "         };"
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

  if (!base::FeatureList::IsEnabled(
          ::preferences::features::kBraveBackgroundVideoPlayback) &&
      !prefs->GetBoolean(kBackgroundVideoPlaybackEnabled))
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
  if (!IsYouTubeDomain(web_contents()->GetLastCommittedURL())) {
    return;
  }
  if (IsBackgroundVideoPlaybackEnabled(web_contents())) {
    web_contents()->GetPrimaryMainFrame()->ExecuteJavaScript(
        k_youtube_background_playback_script, base::NullCallback());
  }
}

namespace chrome {
namespace android {

mojo::AssociatedRemote<script_injector::mojom::ScriptInjector> GetRemote(
    content::RenderFrameHost* rfh) {
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&script_injector_remote);
  return script_injector_remote;
}
void JNI_BackgroundVideoPlaybackTabHelper_SendOrientationChangeEvent(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents,
    jboolean is_full_screen) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  // Injecting this script to make youtube video fullscreen on landscape mode
  // and exit fullscreen on portrait mode.
  std::string script;
  if (is_full_screen) {
    script =
        "if(!document.fullscreenElement) {"
        "   var fullscreenBtn = "
        "     document.getElementsByClassName('fullscreen-icon');"
        "   if(fullscreenBtn && fullscreenBtn.length > 0) {"
        "      fullscreenBtn[0].click();"
        "   } else {"
        "     var moviePlayer = document.getElementById('movie_player');"
        "     if (moviePlayer) {"
        "         moviePlayer.click();"
        "     }"
        "     setTimeout(() => {"
        "         var fullscreenBtn = "
        "           document.getElementsByClassName('fullscreen-icon');"
        "         if(fullscreenBtn && fullscreenBtn.length > 0) {"
        "            fullscreenBtn[0].click();"
        "         }"
        "     }, 50);"
        "   }"
        "}";
  } else {
    script =
        "if(document.fullscreenElement) {"
        "   document.exitFullscreen();"
        "}";
  }
  GetRemote(web_contents->GetPrimaryMainFrame())
      ->RequestAsyncExecuteScript(
          ISOLATED_WORLD_ID_BRAVE_INTERNAL, base::UTF8ToUTF16(script),
          blink::mojom::UserActivationOption::kActivate,
          blink::mojom::PromiseResultOption::kAwait, base::NullCallback());
}

jboolean JNI_BackgroundVideoPlaybackTabHelper_IsPlayingMedia(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  content::MediaSessionImpl* media_session_impl =
      content::MediaSessionImpl::Get(web_contents);
  if (!media_session_impl) {
    return false;
  }
  media_session::mojom::MediaSessionInfoPtr current_info =
      media_session_impl->GetMediaSessionInfoSync();
  return current_info->playback_state ==
         media_session::mojom::MediaPlaybackState::kPlaying;
}
}  // namespace android
}  // namespace chrome
WEB_CONTENTS_USER_DATA_KEY_IMPL(BackgroundVideoPlaybackTabHelper);
