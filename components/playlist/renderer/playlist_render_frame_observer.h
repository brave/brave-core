/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "gin/arguments.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace playlist {

class PlaylistRenderFrameObserver final
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PlaylistRenderFrameObserver>,
      public mojom::ScriptConfigurator {
 public:
  PlaylistRenderFrameObserver(content::RenderFrame* frame,
                              int32_t isolated_world_id);

  PlaylistRenderFrameObserver(const PlaylistRenderFrameObserver&) = delete;
  PlaylistRenderFrameObserver& operator=(const PlaylistRenderFrameObserver&) =
      delete;

  void RunScriptsAtDocumentStart();

 private:
  ~PlaylistRenderFrameObserver() override;

  // RenderFrameObserver:
  void OnDestruct() override;
  // mojom::ScriptConfigurator
  void AddMediaSourceAPISuppressor(base::ReadOnlySharedMemoryRegion script) override;
  void AddMediaDetector(base::ReadOnlySharedMemoryRegion script) override;

  void BindScriptConfigurator(
      mojo::PendingAssociatedReceiver<mojom::ScriptConfigurator> receiver);

  bool EnsureConnectedToMediaHandler();
  void OnMediaHandlerDisconnect();

  void Inject(const std::string& script_text,
              v8::Local<v8::Context> context,
              std::vector<v8::Local<v8::Value>> args = {}) const;
  void OnMediaDetected(gin::Arguments* args);

  int32_t isolated_world_id_;
  mojo::AssociatedReceiver<mojom::ScriptConfigurator>
      script_configurator_receiver_{this};
  mojo::Remote<playlist::mojom::PlaylistMediaHandler> media_handler_;
  std::optional<std::string> media_source_api_suppressor_script_;
  std::optional<std::string> media_detector_script_;
  base::WeakPtrFactory<PlaylistRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_RENDER_FRAME_OBSERVER_H_
