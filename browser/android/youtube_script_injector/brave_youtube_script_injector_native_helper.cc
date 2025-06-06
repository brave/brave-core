/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/youtube_script_injector/brave_youtube_script_injector_native_helper.h"

#include "base/android/jni_android.h"
#include "brave/browser/android/youtube_script_injector/jni_headers/BraveYouTubeScriptInjectorNativeHelper_jni.h"
#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"
#include "brave/components/script_injector/common/mojom/script_injector.mojom.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"

namespace {
constexpr char16_t kYoutubeFullscreen[] =
    uR"(
(function() {
  function triggerFullscreen() {
    // Always play video before entering fullscreen mode.
    document.querySelector("video.html5-main-video")?.play();

    // Check if the video is not in fullscreen mode already.
    if (!document.fullscreenElement) {
      // Create a MutationObserver to watch for changes in the DOM.
      const observer = new MutationObserver((_mutationsList, observer) => {
        var fullscreenBtn = document.querySelector("button.fullscreen-icon");
        var videoPlayer = document.querySelector("video.html5-main-video");
        if (fullscreenBtn && videoPlayer) {
          observer.disconnect()
          delayedPlayAndClick(fullscreenBtn, videoPlayer);
        }
      });

      var fullscreenBtn = document.querySelector("button.fullscreen-icon");
      var videoPlayer = document.querySelector("video.html5-main-video");
      // Check if fullscreen button and video are available.
      if (fullscreenBtn && videoPlayer) {
       delayedPlayAndClick(fullscreenBtn, videoPlayer);
      } else {
        // When fullscreen button is not available
        // clicking the movie player resume the UI.
        var moviePlayer = document.getElementById("movie_player");
        if (moviePlayer) {
          // Start observing the DOM.
          observer.observe(document.body, { childList: true, subtree: true });
          // Make sure the player is in focus or responsive.
          moviePlayer.click();
        }
      }
    }
  }

  // Click the fullscreen button and play the video and after a delay 
  // to ensure the video is ready.
  // This is necessary because sometimes (rarely) when switching to fullscreen
  // mode a video might be paused automatically from the backend if the buffer 
  // was not ready.
  // The delay allows the video to load properly before attempting to play it.
  // This is especially important for high quality videos, which may require 
  // some time to buffer before they can be played.
  // The delay is set to 500 milliseconds, which is a reasonable delay for 
  // the videos to be ready for playback.
  function delayedPlayAndClick(fullscreenBtn, videoPlayer) {
    setTimeout(() => {
      videoPlayer.play();
    }, 500);
    fullscreenBtn.click();
  }

  if (document.readyState === "loading") {
    // Loading hasn't finished yet.
    document.addEventListener("DOMContentLoaded", triggerFullscreen);
  } else {
    // `DOMContentLoaded` has already fired.
    triggerFullscreen();
  }
}());
)";

}  // namespace

namespace youtube_script_injector {

// static
jboolean JNI_BraveYouTubeScriptInjectorNativeHelper_IsYouTubeVideo(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);

  auto url = web_contents->GetLastCommittedURL();
  return YouTubeScriptInjectorTabHelper::IsYouTubeVideo(url);
}

// static
void JNI_BraveYouTubeScriptInjectorNativeHelper_SetFullscreen(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);

  content::RenderFrameHost* rfh = web_contents->GetPrimaryMainFrame();
  mojo::AssociatedRemote<script_injector::mojom::ScriptInjector>
      script_injector_remote;
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&script_injector_remote);

  script_injector_remote->RequestAsyncExecuteScript(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL, kYoutubeFullscreen,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kDoNotWait, base::NullCallback());
}

}  // namespace youtube_script_injector
