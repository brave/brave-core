/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_

#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "url/gurl.h"

namespace playlist {

class PlaylistRenderFrameObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PlaylistRenderFrameObserver> {
 public:
  explicit PlaylistRenderFrameObserver(content::RenderFrame* render_frame);

  void RunScriptsAtDocumentStart();

 private:
  ~PlaylistRenderFrameObserver() override;

  void HideMediaSourceAPI();

  // RenderFrameObserver:
  void OnDestruct() override;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
