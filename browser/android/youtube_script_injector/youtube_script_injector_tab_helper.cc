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

// Drives the YouTube player into fullscreen so the caller can follow up with
// Picture in Picture. On a cold load the player and its controls hydrate
// asynchronously, so the fullscreen button may be absent at injection time: the
// script makes a couple of synchronous attempts, then retries as the subtree
// hydrates. Retries prefer a MutationObserver scoped to the player container;
// only when that container does not exist yet do we fall back to short-interval
// polling, so we never observe the entire document. A find timeout stops the
// retries outliving a broken page.
//
// Crucially, success is never inferred from the click or API call. A
// fullscreenchange listener resolves only once the page is actually fullscreen,
// and a short confirm timeout settles the promise as failed otherwise. This
// keeps the reported result honest and guarantees the promise always settles,
// which the browser relies on to clear its pending Picture in Picture request.
constexpr char16_t kYoutubeFullscreen[] =
    uR"(
(function() {
  return new Promise((resolve) => {
    const videoSelector = "video.html5-main-video";
    const fullscreenSelector = "button.fullscreen-icon, "
        + "button.ytp-fullscreen-button, .ytp-fullscreen-button";
    const playerSelector = "#movie_player, .html5-video-player";
    const playerContainerSelector = "#player-container-id, ytm-player, #player";
    // Wait for the player and its controls to hydrate before giving up on
    // finding something to trigger fullscreen with.
    const FIND_TIMEOUT_MS = 30000;
    // Wait for fullscreen to actually engage once we have triggered it. Short,
    // because a real transition lands almost immediately; this bounds how long
    // the browser keeps a pending Picture in Picture request armed.
    const CONFIRM_TIMEOUT_MS = 2000;
    // How often to re-query for the player when no specific container exists to
    // observe. Polling is only used as a fallback so we never watch the whole
    // document; it stops as soon as a trigger fires or the find timeout
    // elapses.
    const POLL_INTERVAL_MS = 100;

    let resolved = false;
    let triggered = false;
    let observer = null;
    let findTimeoutId = 0;
    let confirmTimeoutId = 0;
    let pollIntervalId = 0;

    function isFullscreen() {
      const player = document.querySelector(playerSelector);
      return !!document.fullscreenElement || !!player?.isFullscreen?.();
    }

    // Stop looking for something to trigger fullscreen with: tear down the
    // observer or poll and the find timeout. Leaves the confirm timeout and
    // fullscreenchange listener in place so success can still be observed.
    function stopSearching() {
      if (observer) {
        observer.disconnect();
        observer = null;
      }
      clearInterval(pollIntervalId);
      clearTimeout(findTimeoutId);
    }

    function cleanup() {
      document.removeEventListener('fullscreenchange', onFullscreenChange);
      stopSearching();
      clearTimeout(confirmTimeoutId);
    }

    function resolveOnce(value) {
      if (resolved) {
        return;
      }
      resolved = true;
      cleanup();
      resolve(value);
    }

    // Single source of truth for success: report 'fullscreen_triggered' only
    // once the page has actually entered fullscreen, never on the click or API
    // call alone.
    function onFullscreenChange() {
      if (isFullscreen()) {
        resolveOnce('fullscreen_triggered');
      }
    }
    document.addEventListener('fullscreenchange', onFullscreenChange);

    // Once a trigger has fired, stop searching and start a short clock: if
    // fullscreen has not engaged by the time it expires, report failure. This
    // guarantees the promise always settles, even when an API call silently
    // does nothing or its promise rejects.
    function armConfirmTimeout() {
      if (triggered) {
        return;
      }
      triggered = true;
      stopSearching();
      confirmTimeoutId = setTimeout(
          () => resolveOnce('requestFullscreen_failed'), CONFIRM_TIMEOUT_MS);
    }

    // Fire a single fullscreen trigger if something is available to act on.
    // Returns true once a trigger was fired (or we are already fullscreen) so
    // the caller stops searching. Success is decided by onFullscreenChange, not
    // by this return value.
    function attempt() {
      if (resolved || triggered) {
        return true;
      }
      if (isFullscreen()) {
        resolveOnce('already_fullscreen');
        return true;
      }
      const btn = document.querySelector(fullscreenSelector);
      const video = document.querySelector(videoSelector);
      if (btn && video) {
        if (video.readyState >= 3) {
          video.click();
        }
        btn.click();
        armConfirmTimeout();
        return true;
      }
      // Fallbacks when the button is not in the tree: YouTube's own toggle,
      // then the standard element fullscreen API. Only one fires per run.
      const player = document.querySelector(playerSelector);
      if (player?.toggleFullscreen
          && !player.classList.contains('ytp-fullscreen')) {
        try {
          player.toggleFullscreen();
          armConfirmTimeout();
          return true;
        } catch (e) {}
      }
      const target = player
          || document.querySelector(playerContainerSelector)
          || video;
      if (target?.requestFullscreen) {
        try {
          const request = target.requestFullscreen();
          // Success is observed via fullscreenchange; a rejection fails fast.
          if (request?.catch) {
            request.catch(() => resolveOnce('requestFullscreen_failed'));
          }
          armConfirmTimeout();
          return true;
        } catch (e) {}
      }
      return false;
    }

    function start() {
      if (attempt()) {
        return;
      }
      // Tap to reveal controls that only render on first interaction.
      (document.querySelector(videoSelector)
          || document.querySelector(playerSelector)
          || document.querySelector(playerContainerSelector))?.click();
      if (attempt()) {
        return;
      }
      // Retry as the button or player hydrates. Prefer a MutationObserver
      // scoped to the player container so we watch a small subtree; if that
      // container is not in the tree yet, fall back to short-interval polling
      // rather than observing the entire document.
      const root = document.querySelector(playerContainerSelector);
      if (root) {
        observer = new MutationObserver(() => {
          attempt();
        });
        observer.observe(root, { childList: true, subtree: true });
      } else {
        pollIntervalId = setInterval(attempt, POLL_INTERVAL_MS);
      }
      // Only reached while nothing has been triggered yet: give up if the
      // player never hydrates.
      findTimeoutId = setTimeout(() => resolveOnce('timeout'), FIND_TIMEOUT_MS);
    }

    if (document.readyState === "loading") {
      document.addEventListener("DOMContentLoaded", start, { once: true });
    } else {
      start();
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

// Only the address of this key identifies the user data; the string value is
// incidental and never read. This follows the Chromium SupportsUserData key
// idiom, e.g. kBackgroundSyncUserDataKey in
// content/browser/background_sync/background_sync_manager.cc.
const char kPictureInPictureRequestKey[] = "PictureInPictureRequest";

// Marks a NavigationEntry as having a pending Picture-in-Picture request. The
// state is attached to the NavigationEntry rather than the tab helper so its
// lifetime matches the page the user acted on: same document navigations,
// back/forward and reloads each carry their own entry, so a request can never
// leak onto a different page. This is what keeps fullscreen and
// Picture-in-Picture in lockstep across the async gap between injecting the
// script and the media actually going fullscreen.
// Callers pass the last committed NavigationEntry, which is contractually
// non-null in WebContentsObserver callbacks once the FrameTree is initialized.
void SetPictureInPictureRequested(content::NavigationEntry& entry,
                                  bool requested) {
  if (requested) {
    entry.SetUserData(kPictureInPictureRequestKey,
                      std::make_unique<base::SupportsUserData::Data>());
  } else {
    entry.RemoveUserData(kPictureInPictureRequestKey);
  }
}

bool IsPictureInPictureRequested(content::NavigationEntry& entry) {
  return entry.GetUserData(kPictureInPictureRequestKey);
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
  if (navigation_handle->IsInPrimaryMainFrame() &&
      navigation_handle->HasCommitted()) {
    // A new entry committed in the main frame (cross document or same
    // document). Drop the pipe bound to the previous document and clear any
    // pending request up front, so a back/forward or same document navigation
    // never re-enters Picture-in-Picture for a request that belonged to a
    // different page.
    script_injector_remote_.reset();
    SetPictureInPictureRequested(
        *web_contents()->GetController().GetLastCommittedEntry(), false);
  }
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

  // Arm the request against the page the user is looking at, then ask the
  // renderer to go fullscreen. MediaEffectivelyFullscreenChanged() reads this
  // back to decide whether to follow up with Picture-in-Picture.
  //
  // We deliberately do not block a repeated injection on an already armed
  // request: the injected script is idempotent (it does nothing and resolves
  // 'already_fullscreen' when the page is already fullscreen), so tapping again
  // is safe, and not gating keeps the button working even if a prior request
  // was left armed by an edge case we cannot observe here.
  SetPictureInPictureRequested(
      *web_contents()->GetController().GetLastCommittedEntry(), true);
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
  const std::string* result = value.GetIfString();
  if (result && *result == "fullscreen_triggered") {
    // Fullscreen is on its way; MediaEffectivelyFullscreenChanged() will pick
    // up the request and enter Picture-in-Picture.
    return;
  }
  if (result && *result == "already_fullscreen") {
    // The page was already fullscreen when the script ran, so there is no
    // fullscreen transition and MediaEffectivelyFullscreenChanged() never
    // fires. The prerequisite for PiP is already met, so enter it directly
    // here; otherwise the armed request would fall through to the failure path
    // below and the user would get no PiP despite the page being fullscreen.
    MaybeEnterPictureInPicture();
    return;
  }
  // The script could not put the page into fullscreen (no player, timeout,
  // etc.), so clear the request: PiP must not be entered, and the user is free
  // to try again on the same page.
  SetPictureInPictureRequested(
      *web_contents()->GetController().GetLastCommittedEntry(), false);
}

void YouTubeScriptInjectorTabHelper::MediaEffectivelyFullscreenChanged(
    bool is_fullscreen) {
  if (!is_fullscreen) {
    return;
  }
  // Fullscreen was reached, most likely off the back of our request. Enter PiP
  // if a request is still armed for the page currently showing.
  MaybeEnterPictureInPicture();
}

void YouTubeScriptInjectorTabHelper::MaybeEnterPictureInPicture() {
  // Only act when a request is still armed for the page currently showing. If
  // the user navigated in the meantime, the new entry carries no request and we
  // leave it alone, keeping fullscreen and PiP consistent (both or neither).
  content::NavigationEntry& entry =
      *web_contents()->GetController().GetLastCommittedEntry();
  if (!IsPictureInPictureRequested(entry)) {
    return;
  }

  // Consume the request as we act on it. PiP here is a native Android activity
  // mode, so the upstream MediaPictureInPictureChanged() signal never fires to
  // clear it; clearing it here is what lets the button work again after
  // returning from a PiP session.
  SetPictureInPictureRequested(entry, false);

  if (web_contents()->GetVisibility() == content::Visibility::VISIBLE) {
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
      *web_contents()->GetController().GetLastCommittedEntry(), false);
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
