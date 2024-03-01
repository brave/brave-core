/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_

#include <optional>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

namespace playlist {

class PlaylistRenderFrameObserver final
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PlaylistRenderFrameObserver>,
      public mojom::PlaylistRenderFrameObserverConfigurator {
 public:
  PlaylistRenderFrameObserver(content::RenderFrame* frame,
                              int32_t isolated_world_id);

  PlaylistRenderFrameObserver(const PlaylistRenderFrameObserver&) = delete;
  PlaylistRenderFrameObserver& operator=(const PlaylistRenderFrameObserver&) =
      delete;

  void RunScriptsAtDocumentStart();
  void RunScriptsAtDocumentEnd();

 private:
  ~PlaylistRenderFrameObserver() override;

  // RenderFrameObserver:
  void OnDestruct() override;

  // mojom::PlaylistRenderFrameObserverConfigurator
  void AddMediaSourceAPISuppressor(
      const std::string& media_source_api_suppressor) override;
  void AddMediaDetector(const std::string& media_detector) override;

  void BindConfigurator(
      mojo::PendingAssociatedReceiver<
          mojom::PlaylistRenderFrameObserverConfigurator> receiver);

  const mojo::AssociatedRemote<mojom::PlaylistMediaResponder>&
  GetMediaResponder();

  void Inject(const std::string& script_text,
              v8::Local<v8::Context> context,
              std::vector<v8::Local<v8::Value>> args = {}) const;
  void OnMediaDetected(base::Value::List media);

  int32_t isolated_world_id_;
  mojo::AssociatedReceiver<mojom::PlaylistRenderFrameObserverConfigurator>
      configurator_receiver_{this};
  mojo::AssociatedRemote<mojom::PlaylistMediaResponder> media_responder_;
  std::optional<std::string> media_source_api_suppressor_;
  std::optional<std::string> media_detector_;
  base::WeakPtrFactory<PlaylistRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
