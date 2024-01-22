/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace playlist {

class PlaylistJSHandler;

class PlaylistRenderFrameObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PlaylistRenderFrameObserver> {
 public:
  PlaylistRenderFrameObserver(content::RenderFrame* render_frame,
                              const int32_t isolated_world_id);

  void RunScriptsAtDocumentStart();

 private:
  ~PlaylistRenderFrameObserver() override;

  void HideMediaSourceAPI();
  void InstallMediaDetector();

  // RenderFrameObserver:
  void OnDestruct() override;
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  const int32_t isolated_world_id_ = 0;

  std::unique_ptr<PlaylistJSHandler> javascript_handler_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
