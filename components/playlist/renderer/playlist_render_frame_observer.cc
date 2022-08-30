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

PlaylistRenderFrameObserver::~PlaylistRenderFrameObserver() {}

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

}  // namespace playlist
