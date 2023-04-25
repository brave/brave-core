/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <vector>

#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/renderer/playlist_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/web_isolated_world_info.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* render_frame,
    const int32_t isolated_world_id)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(render_frame),
      isolated_world_id_(isolated_world_id),
      javascript_handler_(std::make_unique<PlaylistJSHandler>(render_frame)) {}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  const auto& blink_preferences = render_frame()->GetBlinkPreferences();
  if (blink_preferences.hide_media_src_api) {
    HideMediaSourceAPI();
  }

  if (blink_preferences.should_detect_media_files) {
    InstallMediaDetector();
  }
}

void PlaylistRenderFrameObserver::HideMediaSourceAPI() {
  // Hide MediaSource API so that we can get downloadable URL from the page.
  // Otherwise, we get "blob: " url which we can't handle.
  // This script is from
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/PlaylistSwizzler.js
  static const char16_t kScriptToHideMediaSourceAPI[] =
      uR"-(
    (function() {
      // Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
      if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
        window.MediaSource = null;
        window.WebKitMediaSource = null;
        delete window.MediaSource;
        delete window.WebKitMediaSource;
      }
    })();
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebScriptSource(
      blink::WebString::FromUTF16(kScriptToHideMediaSourceAPI)));
}

void PlaylistRenderFrameObserver::InstallMediaDetector() {
  DVLOG(2) << __FUNCTION__;

  static const char16_t kScriptToDetectVideoAndAudio[] =
      uR"-(
    (function() {
      // Firstly, we try to get find all <video> or <audio> tags periodically,
      // for a a while from the start up. If we find them, then we attach 
      // MutationObservers to them to detect source URL.
      // After a given amount of time, we do this in requestIdleCallback().
      // Note that there's a global object named |pl_worker|. This worker is
      // created and bound by PlaylistJSHandler.

      const mutationSources = new Set();
      const mutationObserver = new MutationObserver(mutations => {
          mutations.forEach(mutation => { pl_worker.onMediaUpdated(mutation.target.src); })
      });
      const findNewMediaAndObserveMutation = () => {
          return document.querySelectorAll('video, audio').forEach((mediaNode) => {
              if (mutationSources.has(mediaNode)) return

              mutationSources.add(mediaNode)
              pl_worker.onMediaUpdated(mediaNode.src)
              mutationObserver.observe(mediaNode, { attributeFilter: ['src'] })
          });
      }

      const pollingIntervalId = window.setInterval(findNewMediaAndObserveMutation, 1000);
      window.setTimeout(() => {
          window.clearInterval(pollingIntervalId)
          window.requestIdleCallback(findNewMediaAndObserveMutation)
          // TODO(sko) We might want to check if idle callback is waiting too long.
          // In that case, we should get back to the polling style. And also, this
          // time could be too long for production.
      }, 20000)
    })();
    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional()) {
    return;
  }

  web_frame->ExecuteScriptInIsolatedWorld(
      isolated_world_id_,
      blink::WebScriptSource(
          blink::WebString::FromUTF16(kScriptToDetectVideoAndAudio)),
      blink::BackForwardCacheAware::kAllow);
}

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (world_id != isolated_world_id_) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << "Will add Playlist worker object to the frame";
  javascript_handler_->AddWorkerObjectToFrame(context);
}

}  // namespace playlist
