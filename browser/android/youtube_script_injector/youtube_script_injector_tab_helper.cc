/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"

#include <memory>
#include <string>

#include "base/android/android_info.h"
#include "base/feature_list.h"
#include "base/supports_user_data.h"
#include "brave/browser/android/youtube_script_injector/brave_youtube_script_injector_native_helper.h"
#include "brave/browser/android/youtube_script_injector/features.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace {
constexpr char16_t kYoutubeBackgroundPlayback[] =
    uR"(
(function() {
  if (document._addEventListener === undefined) {
    document._addEventListener = document.addEventListener;
    document.addEventListener = function(a, b, c) {
      if (a != 'visibilitychange') {
        document._addEventListener(a, b, c);
      }
    };
  }

  // Override document.visibilityState to always return 'visible'
  Object.defineProperty(document, 'visibilityState', {
    configurable: true,
    get: function() { return 'visible'; }
  });
}());
)";

constexpr char16_t kYoutubePictureInPictureSupport[] =
    uR"(
(function() {
  // Function to modify the flags if the target object exists.
  function modifyYtcfgFlags() {
    const config = window.ytcfg.get("WEB_PLAYER_CONTEXT_CONFIGS")
      ?.WEB_PLAYER_CONTEXT_CONFIG_ID_MWEB_WATCH
    if (config && config.serializedExperimentFlags && typeof config
      .serializedExperimentFlags === 'string') {
      let flags = config.serializedExperimentFlags;

      // Replace target flags.
      flags = flags
        .replace(
          "html5_picture_in_picture_blocking_ontimeupdate=true",
          "html5_picture_in_picture_blocking_ontimeupdate=false")
        .replace("html5_picture_in_picture_blocking_onresize=true",
          "html5_picture_in_picture_blocking_onresize=false")
        .replace(
          "html5_picture_in_picture_blocking_document_fullscreen=true",
          "html5_picture_in_picture_blocking_document_fullscreen=false"
        )
        .replace(
          "html5_picture_in_picture_blocking_standard_api=true",
          "html5_picture_in_picture_blocking_standard_api=false")
        .replace("html5_picture_in_picture_logging_onresize=true",
          "html5_picture_in_picture_logging_onresize=false");

      // Assign updated flags back to config.
      config.serializedExperimentFlags = flags;
    }
  }

  if (window.ytcfg) {
    modifyYtcfgFlags();
  } else {
    document.addEventListener('load', (event) => {
      const target = event.target;
      if (target.tagName === 'SCRIPT' && window.ytcfg) {
        // Check and modify flags when a new script is added.
        modifyYtcfgFlags();
      }
    }, true);
  }
}());
)";

constexpr char16_t kYoutubeFullscreen[] =
    uR"(
(function() {
  return new Promise((resolve) => {
    const videoPlaySelector = "video.html5-main-video";
    const fullscreenSelector = "button.fullscreen-icon";
    function triggerFullscreen() {
      // Check if the video is not in fullscreen mode already.
      if (!document.fullscreenElement) {
        var fullscreenBtn = document.querySelector(fullscreenSelector);
        var videoPlayer = document.querySelector(videoPlaySelector);
        // Check if fullscreen button and video are available.
        if (fullscreenBtn && videoPlayer) {
         requestFullscreen(fullscreenBtn, resolve, videoPlayer);
        } else {
          // When fullscreen button is not available
          // clicking the movie player resume the UI.
          var playerContainer = document.getElementById("player-container-id");
          if (videoPlayer && playerContainer) {
            let observerTimeout;
            // Create a MutationObserver to watch for changes in the DOM.
            const observer = new MutationObserver(
            (_mutationsList, observer) => {
              var fullscreenBtn = document.querySelector(fullscreenSelector);
              var videoPlayer = document.querySelector(videoPlaySelector);
              if (fullscreenBtn && videoPlayer) {
                clearTimeout(observerTimeout);
                observer.disconnect()
                requestFullscreen(fullscreenBtn, resolve, videoPlayer);
              }
            });
            // Auto-disconnect the observer after 30 seconds,
            // a reasonable duration picked after some testing.
            observerTimeout = setTimeout(() => {
              observer.disconnect();
              resolve('timeout');
            }, 30000);
            // Start observing the DOM.
            observer.observe(playerContainer, {
              childList: true, subtree: true
            });
            // Make sure the player is in focus or responsive.
            videoPlayer.click();
          } else {
            // No fullscreen elements found, resolve immediately
            resolve('no_elements');
          }
        }
      } else {
        // Already in fullscreen, resolve immediately
        resolve('already_fullscreen');
      }
    }
    // Attempts to request fullscreen mode for the given movie player element.
    // Resolves with 'fullscreen_triggered' if successful, or
    // 'requestFullscreen_failed' if the request fails.
    function requestFullscreen(fullscreenBtn, resolve, videoPlayer) {
      if (videoPlayer.readyState >= 3) {
        videoPlayer.click();
        clickFullscreenButton(fullscreenBtn, resolve);
      } else {
        videoPlayer.addEventListener("canplay", () => {
          videoPlayer.click();
          clickFullscreenButton(fullscreenBtn, resolve);
        }, { once: true });
      }
    }
    function clickFullscreenButton(fullscreenBtn, resolve) {
      if (fullscreenBtn && !document.hidden) {
        fullscreenBtn.click();
        resolve('fullscreen_triggered');
      } else {
        resolve('requestFullscreen_failed');
      }
    }
    if (document.readyState === "loading") {
      // Loading hasn't finished yet.
      document.addEventListener("DOMContentLoaded",
      triggerFullscreen, { once: true });
    } else {
      // `DOMContentLoaded` has already fired.
      triggerFullscreen();
    }
  });
}());
)";

bool IsBackgroundVideoPlaybackEnabled(content::WebContents* contents) {
  PrefService* prefs =
      static_cast<Profile*>(contents->GetBrowserContext())->GetPrefs();

  return (base::FeatureList::IsEnabled(
              ::preferences::features::kBraveBackgroundVideoPlayback) &&
          prefs->GetBoolean(kBackgroundVideoPlaybackEnabled));
}

// Mirrors the SDK gate that Java's PictureInPicture.isEnabled() applies before
// any PiP entry attempt (see
// chrome/android/java/src/.../media/PictureInPicture.java). PiP is
// hard-disabled on Android < R to dodge a framework crash when entering PiP
// immediately after exiting it.
bool IsAndroidPictureInPictureSupported() {
  return base::android::android_info::sdk_int() >=
         base::android::android_info::SDK_VERSION_R;
}

// Marks a NavigationEntry as having a pending Picture-in-Picture request. The
// state is attached to the NavigationEntry rather than the tab helper so its
// lifetime matches the page the user acted on: same document navigations,
// back/forward and reloads each carry their own entry, so a request can never
// leak onto a different page. This is what keeps fullscreen and
// Picture-in-Picture in lockstep across the async gap between injecting the
// script and the media actually going fullscreen.
class PictureInPictureRequest : public base::SupportsUserData::Data {};

// Only the address of this key identifies the user data; the string value is
// incidental and never read. This follows the Chromium SupportsUserData key
// idiom, e.g. kBackgroundSyncUserDataKey in
// content/browser/background_sync/background_sync_manager.cc.
const char kPictureInPictureRequestKey[] = "PictureInPictureRequest";

// Callers pass the last committed NavigationEntry, which is contractually
// non-null in WebContentsObserver callbacks once the FrameTree is initialized.
void SetPictureInPictureRequested(content::NavigationEntry* entry,
                                  bool requested) {
  if (requested) {
    entry->SetUserData(kPictureInPictureRequestKey,
                       std::make_unique<PictureInPictureRequest>());
  } else {
    entry->RemoveUserData(kPictureInPictureRequestKey);
  }
}

bool IsPictureInPictureRequested(content::NavigationEntry* entry) {
  return entry->GetUserData(kPictureInPictureRequestKey);
}

}  // namespace

YouTubeScriptInjectorTabHelper::YouTubeScriptInjectorTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<YouTubeScriptInjectorTabHelper>(*contents) {}

YouTubeScriptInjectorTabHelper::~YouTubeScriptInjectorTabHelper() {}

void YouTubeScriptInjectorTabHelper::PrimaryPageChanged(content::Page& page) {
  script_injector_remote_.reset();
}

void YouTubeScriptInjectorTabHelper::RenderFrameHostChanged(
    content::RenderFrameHost* old_host,
    content::RenderFrameHost*) {
  if (old_host && old_host->IsInPrimaryMainFrame()) {
    script_injector_remote_.reset();
  }
}

void YouTubeScriptInjectorTabHelper::RenderFrameDeleted(
    content::RenderFrameHost* rfh) {
  if (rfh->IsInPrimaryMainFrame()) {
    script_injector_remote_.reset();
  }
}

void YouTubeScriptInjectorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }
  // A new entry committed in the main frame (cross document or same document).
  // Drop the pipe bound to the previous document and clear any pending request
  // up front, so a back/forward or same document navigation never re-enters
  // Picture-in-Picture for a request that belonged to a different page.
  script_injector_remote_.reset();
  SetPictureInPictureRequested(
      web_contents()->GetController().GetLastCommittedEntry(), false);
}

void YouTubeScriptInjectorTabHelper::PrimaryMainDocumentElementAvailable() {
  content::WebContents* contents = web_contents();
  if (!IsYouTubeDomain()) {
    return;
  }
  content::RenderFrameHost::AllowInjectingJavaScript();
  if (IsBackgroundVideoPlaybackEnabled(contents)) {
    contents->GetPrimaryMainFrame()->ExecuteJavaScript(
        kYoutubeBackgroundPlayback, base::NullCallback());
  }
  if (base::FeatureList::IsEnabled(
          ::preferences::features::kBravePictureInPictureForYouTubeVideos) &&
      IsAndroidPictureInPictureSupported()) {
    contents->GetPrimaryMainFrame()->ExecuteJavaScript(
        kYoutubePictureInPictureSupport, base::NullCallback());
  }
}

void YouTubeScriptInjectorTabHelper::
    MaybeSetFullScreenAndPictureInPictureMode() {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  if (!rfh || !rfh->IsRenderFrameLive()) {
    return;
  }

  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  // Tapping the button again while a request is still pending for this page is
  // a no-op: the flag is one-shot and cleared either when PiP engages or when
  // the script reports it could not reach fullscreen.
  if (IsPictureInPictureRequested(entry)) {
    return;
  }

  // Record the request against the page the user is looking at, then ask the
  // renderer to go fullscreen. MediaEffectivelyFullscreenChanged() reads this
  // back to decide whether to follow up with Picture-in-Picture.
  SetPictureInPictureRequested(entry, true);
  EnsureBound(rfh);
  script_injector_remote_->RequestAsyncExecuteScript(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL, kYoutubeFullscreen,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(
          &YouTubeScriptInjectorTabHelper::OnFullscreenScriptComplete,
          weak_factory_.GetWeakPtr()));
}

bool YouTubeScriptInjectorTabHelper::IsYouTubeDomain(bool mobileOnly) const {
  const GURL& url = web_contents()->GetLastCommittedURL();
  if (!url.is_valid() || url.is_empty()) {
    return false;
  }

  // Check if domain is youtube.com (including subdomains).
  if (!net::registry_controlled_domains::SameDomainOrHost(
          url, GURL("https://www.youtube.com"),
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return false;
  }

  // If mobileOnly is true, require host to be exactly "m.youtube.com"
  // (case-insensitive).
  if (mobileOnly) {
    if (!base::EqualsCaseInsensitiveASCII(url.host(), "m.youtube.com")) {
      return false;
    }
  }

  return true;
}

bool YouTubeScriptInjectorTabHelper::IsYouTubeVideo(bool mobileOnly) const {
  if (!IsYouTubeDomain(mobileOnly)) {
    return false;
  }

  const GURL& url = web_contents()->GetLastCommittedURL();

  // Check if path is exactly "/watch" (case sensitive).
  std::string_view path = url.path();
  constexpr std::string_view watch_path = "/watch";
  if (path != watch_path) {
    return false;
  }

  // Check if query exists and contains a non-empty "v" parameter.
  std::string_view query = url.query();
  if (query.empty()) {
    return false;
  }

  // Key-value pairs are '&' delimited and the keys/values are '=' delimited.
  // Example: "https://www.youtube.com/watch?v=abcdefg&somethingElse=12345".
  std::string video_id;
  url::Component query_component(0, static_cast<int>(query.size()));
  url::Component key, value;
  while (url::ExtractQueryKeyValue(query, &query_component, &key, &value)) {
    if (query.substr(key.begin, key.len) == "v") {
      video_id = std::string(query.substr(value.begin, value.len));
      base::TrimWhitespaceASCII(video_id, base::TRIM_ALL, &video_id);
      break;
    }
  }

  return !video_id.empty();
}

void YouTubeScriptInjectorTabHelper::OnFullscreenScriptComplete(
    base::Value value) {
  if (value.is_string() && value.GetString() == "fullscreen_triggered") {
    // Fullscreen is on its way; MediaEffectivelyFullscreenChanged() will pick
    // up the request and enter Picture-in-Picture.
    return;
  }
  // The script could not put the page into fullscreen (no player, timeout,
  // etc.), so clear the request: PiP must not be entered, and the user is free
  // to try again on the same page.
  SetPictureInPictureRequested(
      web_contents()->GetController().GetLastCommittedEntry(), false);
}

void YouTubeScriptInjectorTabHelper::MediaEffectivelyFullscreenChanged(
    bool is_fullscreen) {
  if (!is_fullscreen) {
    return;
  }

  // Only follow up with PiP when fullscreen was reached off the back of our
  // request for the page currently showing. If the user navigated in the
  // meantime, the new entry carries no request and we leave it alone, keeping
  // fullscreen and PiP consistent (both or neither).
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!IsPictureInPictureRequested(entry)) {
    return;
  }

  // Consume the one-shot request as we act on it. PiP here is a native Android
  // activity mode, so the upstream MediaPictureInPictureChanged() signal never
  // fires to clear it; clearing on the fullscreen transition is what lets the
  // button work again after returning from a PiP session.
  SetPictureInPictureRequested(entry, false);

  // Skip PiP for a backgrounded tab, and on Android versions where entering it
  // is unsafe.
  if (web_contents()->GetVisibility() == content::Visibility::VISIBLE &&
      IsAndroidPictureInPictureSupported()) {
    ::youtube_script_injector::EnterPictureInPicture(web_contents());
  }
}

void YouTubeScriptInjectorTabHelper::MediaPictureInPictureChanged(
    bool is_picture_in_picture) {
  if (!is_picture_in_picture) {
    return;
  }
  // Defensive backstop for the Web Picture-in-Picture path (video element /
  // MediaSession PiP). The YouTube button uses native Android activity PiP,
  // which does not route through here, so MediaEffectivelyFullscreenChanged()
  // owns clearing the request in practice.
  SetPictureInPictureRequested(
      web_contents()->GetController().GetLastCommittedEntry(), false);
}

bool YouTubeScriptInjectorTabHelper::IsPictureInPictureAvailable() const {
  return base::FeatureList::IsEnabled(
             preferences::features::kBravePictureInPictureForYouTubeVideos) &&
         IsYouTubeVideo(true) && web_contents() &&
         web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame();
}

void YouTubeScriptInjectorTabHelper::EnsureBound(
    content::RenderFrameHost* rfh) {
  DCHECK(rfh);
  DCHECK(rfh->IsRenderFrameLive());

  script_injector_remote_.reset();
  rfh->GetRemoteAssociatedInterfaces()->GetInterface(&script_injector_remote_);
  script_injector_remote_.reset_on_disconnect();
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(YouTubeScriptInjectorTabHelper);
