/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/youtube_script_injector/youtube_script_injector_tab_helper.h"

#include <memory>
#include <string>

#include "base/feature_list.h"
#include "base/supports_user_data.h"
#include "brave/browser/android/youtube_script_injector/brave_youtube_script_injector_native_helper.h"
#include "brave/browser/android/youtube_script_injector/features.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/content/public/browser/fullscreen_page_data.h"
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
    const fullscreenSelector = "button.fullscreen-icon, "
        + "button.ytp-fullscreen-button, .ytp-fullscreen-button";
    const playerSelector = "#movie_player, .html5-video-player";
    const playerContainerSelector = "#player-container-id, ytm-player, #player";
    const maxAttempts = 6;
    const retryDelayMs = 350;
    function isFullscreen(player) {
      player = player || document.querySelector(playerSelector);
      const isPlayerFullscreen = player?.isFullscreen?.();
      return !!document.fullscreenElement || !!isPlayerFullscreen;
    }
    function hasFullscreenClass(player) {
      return !!(player && player.classList.contains("ytp-fullscreen"));
    }
    function revealControls(videoPlayer, player, playerContainer) {
      if (videoPlayer) {
        videoPlayer.click();
      } else if (player) {
        player.click();
      } else if (playerContainer) {
        playerContainer.click();
      }
    }
    function triggerFullscreen(attempt) {
      const player = document.querySelector(playerSelector);
      if (isFullscreen(player)) {
        resolve('already_fullscreen');
        return;
      }

      const fullscreenBtn = document.querySelector(fullscreenSelector);
      const videoPlayer = document.querySelector(videoPlaySelector);
      const playerContainer = document.querySelector(playerContainerSelector);
      if (videoPlayer) {
        const playResult = videoPlayer.play?.();
        if (playResult) {
          playResult.catch(() => {});
        }
      }

      if (fullscreenBtn && videoPlayer) {
        requestFullscreen(fullscreenBtn, videoPlayer, attempt);
        return;
      }

      // YouTube sometimes hides the fullscreen button until the player
      // receives a tap (e.g. on fresh load). Reveal controls then retry.
      revealControls(videoPlayer, player, playerContainer);
      if (requestYoutubeFullscreen(player, attempt)) {
        return;
      }
      if (requestElementFullscreen(player || playerContainer || videoPlayer,
              attempt)) {
        return;
      }
      scheduleRetry(attempt);
    }
    function requestFullscreen(fullscreenBtn, videoPlayer, attempt) {
      if (videoPlayer.readyState >= 3) {
        videoPlayer.click();
        clickFullscreenButton(fullscreenBtn, attempt);
      } else {
        videoPlayer.addEventListener("canplay", () => {
          videoPlayer.click();
          clickFullscreenButton(fullscreenBtn, attempt);
        }, { once: true });
        scheduleRetry(attempt);
      }
    }
    function clickFullscreenButton(fullscreenBtn, attempt) {
      if (isFullscreen()) {
        resolve('already_fullscreen');
        return;
      }
      if (fullscreenBtn) {
        fullscreenBtn.click();
        resolve('fullscreen_triggered');
      } else {
        scheduleRetry(attempt);
      }
    }
    function requestYoutubeFullscreen(player, attempt) {
      if (isFullscreen(player)) {
        resolve('already_fullscreen');
        return true;
      }
      if (!player || !player.toggleFullscreen || hasFullscreenClass(player)) {
        return false;
      }

      try {
        player.toggleFullscreen();
        if (requestElementFullscreen(player, attempt)) {
          return true;
        }
        waitForFullscreen(attempt);
        return true;
      } catch (e) {
        return false;
      }
    }
    function waitForFullscreen(attempt) {
      setTimeout(() => {
        if (isFullscreen()) {
          resolve('fullscreen_triggered');
          return;
        }
        scheduleRetry(attempt);
      }, retryDelayMs);
    }
    function requestElementFullscreen(element, attempt) {
      if (isFullscreen()) {
        resolve('already_fullscreen');
        return true;
      }

      if (!element?.requestFullscreen) {
        return false;
      }

      try {
        const result = element.requestFullscreen();
        if (result && result.then) {
          result.then(() => resolve('fullscreen_triggered'))
              .catch(() => scheduleRetry(attempt));
        } else {
          resolve('fullscreen_triggered');
        }
        return true;
      } catch (e) {
        return false;
      }
    }
    function scheduleRetry(attempt) {
      if (attempt >= maxAttempts) {
        resolve('no_elements');
        return;
      }
      setTimeout(() => triggerFullscreen(attempt + 1), retryDelayMs);
    }
    if (document.readyState === "loading") {
      // Loading hasn't finished yet.
      document.addEventListener("DOMContentLoaded",
      () => triggerFullscreen(0), { once: true });
    } else {
      // `DOMContentLoaded` has already fired.
      triggerFullscreen(0);
    }
  });
}());
)";

constexpr char16_t kYoutubeExitFullscreen[] =
    uR"(
(function() {
  if (!document.fullscreenElement || !document.exitFullscreen) {
    return;
  }

  document.exitFullscreen().catch(() => {
    const fullscreenBtn = document.querySelector("button.fullscreen-icon");
    if (fullscreenBtn && document.fullscreenElement) {
      fullscreenBtn.click();
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

}  // namespace

YouTubeScriptInjectorTabHelper::YouTubeScriptInjectorTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<YouTubeScriptInjectorTabHelper>(*contents) {}

YouTubeScriptInjectorTabHelper::~YouTubeScriptInjectorTabHelper() {}

void YouTubeScriptInjectorTabHelper::PrimaryPageChanged(content::Page& page) {
  script_injector_remote_.reset();
  bound_rfh_id_ = {};
  SetFullscreenRequested(false);
}

void YouTubeScriptInjectorTabHelper::RenderFrameDeleted(
    content::RenderFrameHost* rfh) {
  if (rfh->GetGlobalId() == bound_rfh_id_) {
    script_injector_remote_.reset();
    bound_rfh_id_ = {};
    SetFullscreenRequested(false);
  }
}

void YouTubeScriptInjectorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsSameDocument() &&
      navigation_handle->IsInMainFrame() && navigation_handle->HasCommitted()) {
    SetFullscreenRequested(false);
  }
}

void YouTubeScriptInjectorTabHelper::PrimaryMainDocumentElementAvailable() {
  SetFullscreenRequested(false);
  content::WebContents* contents = web_contents();
  // Filter only YouTube videos.
  if (!IsYouTubeDomain()) {
    return;
  }
  content::RenderFrameHost::AllowInjectingJavaScript();
  if (IsBackgroundVideoPlaybackEnabled(contents)) {
    contents->GetPrimaryMainFrame()->ExecuteJavaScript(
        kYoutubeBackgroundPlayback, base::NullCallback());
  }
  if (base::FeatureList::IsEnabled(
          ::preferences::features::kBravePictureInPictureForYouTubeVideos)) {
    contents->GetPrimaryMainFrame()->ExecuteJavaScript(
        kYoutubePictureInPictureSupport, base::NullCallback());
  }
}

void YouTubeScriptInjectorTabHelper::MediaEffectivelyFullscreenChanged(
    bool is_fullscreen) {
  if (is_fullscreen && HasFullscreenBeenRequested()) {
    SetFullscreenRequested(false);
    if (web_contents()->GetVisibility() == content::Visibility::VISIBLE) {
      ::youtube_script_injector::EnterPictureInPicture(web_contents());
    }
  }
}

void YouTubeScriptInjectorTabHelper::MaybeSetFullscreen() {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  // Check if fullscreen has already been requested for this page.
  if (!rfh || !rfh->IsRenderFrameLive() || HasFullscreenBeenRequested()) {
    return;
  }

  // Mark fullscreen as requested for this page
  SetFullscreenRequested(true);
  EnsureBound(rfh);
  script_injector_remote_->RequestAsyncExecuteScript(
      ISOLATED_WORLD_ID_BRAVE_INTERNAL, kYoutubeFullscreen,
      blink::mojom::UserActivationOption::kActivate,
      blink::mojom::PromiseResultOption::kAwait,
      base::BindOnce(
          &YouTubeScriptInjectorTabHelper::OnFullscreenScriptComplete,
          weak_factory_.GetWeakPtr(), rfh->GetGlobalFrameToken()));
}

void YouTubeScriptInjectorTabHelper::MaybeExitFullscreen() {
  content::RenderFrameHost* rfh = web_contents()->GetPrimaryMainFrame();
  if (!rfh || !rfh->IsRenderFrameLive()) {
    return;
  }

  content::RenderFrameHost::AllowInjectingJavaScript();
  rfh->ExecuteJavaScript(kYoutubeExitFullscreen, base::NullCallback());
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

bool YouTubeScriptInjectorTabHelper::HasFullscreenBeenRequested() const {
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry) {
    return false;
  }

  auto* data = static_cast<content::FullscreenPageData*>(
      entry->GetUserData(content::kFullscreenPageDataKey));
  return data && data->fullscreen_requested();
}

void YouTubeScriptInjectorTabHelper::SetFullscreenRequested(bool requested) {
  content::NavigationEntry* entry =
      web_contents()->GetController().GetLastCommittedEntry();
  if (!entry) {
    return;
  }

  auto* data = static_cast<content::FullscreenPageData*>(
      entry->GetUserData(content::kFullscreenPageDataKey));
  if (data) {
    data->set_fullscreen_requested(requested);
  } else {
    entry->SetUserData(
        content::kFullscreenPageDataKey,
        std::make_unique<content::FullscreenPageData>(requested));
  }
}

void YouTubeScriptInjectorTabHelper::OnFullscreenScriptComplete(
    content::GlobalRenderFrameHostToken token,
    base::Value value) {
  // If the tab is visible, the script result indicates fullscreen was
  // triggered, and the callback is for the current main frame, return early
  // without resetting the fullscreen state. This prevents unnecessary state
  // changes when fullscreen was successfully entered.
  if (web_contents()->GetVisibility() == content::Visibility::VISIBLE &&
      value.is_string() && value.GetString() == "fullscreen_triggered" &&
      token == web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()) {
    return;
  }

  SetFullscreenRequested(false);
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

  if (!script_injector_remote_.is_bound() ||
      !script_injector_remote_.is_connected() ||
      bound_rfh_id_ != rfh->GetGlobalId()) {
    script_injector_remote_.reset();
    bound_rfh_id_ = rfh->GetGlobalId();
    rfh->GetRemoteAssociatedInterfaces()->GetInterface(
        &script_injector_remote_);
    script_injector_remote_.reset_on_disconnect();
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(YouTubeScriptInjectorTabHelper);
