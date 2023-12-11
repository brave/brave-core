/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace playlist {

class PlaylistRenderFrameObserver final
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PlaylistRenderFrameObserver> {
 public:
  PlaylistRenderFrameObserver(content::RenderFrame* render_frame,
                              int32_t isolated_world_id);

  void RunScriptsAtDocumentStart();

 private:
  // RenderFrameObserver:
  ~PlaylistRenderFrameObserver() override;
  void OnDestruct() override;

  bool EnsureConnectedToMediaHandler();
  void OnMediaHandlerDisconnect();

  void HideMediaSourceAPI() const;
  void InstallMediaDetector();

  void OnMediaUpdated(const std::string& page_url);

 private:
  int32_t isolated_world_id_;
  mojo::Remote<playlist::mojom::PlaylistMediaHandler> media_handler_;
  base::WeakPtrFactory<PlaylistRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
