/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/renderer/playlist_render_frame_observer.h"

#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "brave/components/playlist/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace playlist {

PlaylistRenderFrameObserver::PlaylistRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      RenderFrameObserverTracker<PlaylistRenderFrameObserver>(render_frame) {}

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() = default;

void PlaylistRenderFrameObserver::RunScriptsAtDocumentStart() {
  // TODO(sko) This list should be dynamically updated from browser process.
  // For now, we hardcode the list of domains that we want to run scripts at.
  static const base::NoDestructor<std::vector<blink::WebSecurityOrigin>>
      origins{
          {blink::WebSecurityOrigin::Create(GURL("https://www.youtube.com"))}};

  const auto current_origin =
      render_frame()->GetWebFrame()->GetSecurityOrigin();
  if (base::ranges::any_of(*origins, [&current_origin](const auto& origin) {
        return origin.IsSameOriginWith(current_origin);
      })) {
    HideMediaSourceAPI();
  }
}

void PlaylistRenderFrameObserver::OnDestruct() {
  delete this;
}

void PlaylistRenderFrameObserver::HideMediaSourceAPI() {
  // Hide MediaSource API so that we can get downloadable URL from the page.
  // Otherwise, we get "blob: " url which we can't handle.
  // This script is from
  // https://github.com/brave/brave-ios/blob/development/Client/Frontend/UserContent/UserScripts/PlaylistSwizzler.js
  // TODO(sko) This script should be managed by "Component" as it's fragile.
  static const char16_t kScriptToHideMediaSourceAPI[] =
      uR"-(
    (function() {
      const isYoutubeStreaming = () => {
        return window.__isLiveContent__ || !!window.ytplayer?.bootstrapPlayerResponse?.videoDetails?.isLiveContent
      }

      // Stub out the MediaSource API so video players do not attempt to use `blob` for streaming
      if (window.MediaSource || window.WebKitMediaSource || window.HTMLMediaElement && HTMLMediaElement.prototype.webkitSourceAddId) {
        window.__MediaSource__ = window.MediaSource;
        window.__WebKitMediaSource__ = window.WebKitMediaSource;
        window.MediaSource = null;
        window.WebKitMediaSource = null;
        delete window.MediaSource;
        delete window.WebKitMediaSource;

        if (window.__MediaSource__) {
          Object.defineProperties(window, {
            MediaSource: {
              get: function() {
                return isYoutubeStreaming() ? window.__MediaSource__ : null
              }
            }
          });
        }

        if (window.__WebKitMediaSource__) {
          Object.defineProperties(window, {
            WebKitMediaSource: {
              get: function() {
                return isYoutubeStreaming() ? window.__WebKitMediaSource__ : null
              }
            }
          });
        }
      }
    })();


    (function(){
      // Hook fetch API to intercept player information from the response.
      // As youtube is SPA and player data is updated via Fetch API.
      window.__isLiveContent__ = false;
      window.__isLiveContentCache__ = {};

      let originalFetch = window.fetch;
      window.fetch = function(request, data) {
        return new Promise((resolve, reject) => {
          let originalPromise = originalFetch(request, data)
          if (request?.url?.startsWith('https://www.youtube.com/youtubei/v1/player')) {
            // Parse response and mark if it's streaming ahead of the application.
            originalPromise.then(response => { 
                    return Promise.all([response, response.clone().json()])
                  })
                  .then(([response, json, ...rest]) => { 
                    const videoDetail = json?.videoDetails
                    if (videoDetail) {
                      const isLiveContent = !!videoDetail.isLiveContent;
                      window.__isLiveContent__ = isLiveContent;
                      window.__isLiveContentCache__[videoDetail.videoId] = isLiveContent;
                    }
                    resolve(response);
                    })
                  .catch(error => { 
                    reject(error)
                    })
            return;
          }

          originalPromise.then(response => resolve(response))
                         .catch(error => reject(error))
        })
      }

      window.addEventListener('popstate', event => (event) => {
        const videoId = event?.state?.endpoint?.watchEndpoint?.videoId;
        if (videoId)
          window.__isLiveContent__ = window.__isLiveContentCache__[videoId]
      });
    })();

    )-";

  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(blink::WebScriptSource(
      blink::WebString::FromUTF16(kScriptToHideMediaSourceAPI)));
}

}  // namespace playlist
