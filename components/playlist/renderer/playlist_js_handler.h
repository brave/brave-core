/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace playlist {

class PlaylistJSHandler {
 public:
  explicit PlaylistJSHandler(content::RenderFrame* render_frame);
  ~PlaylistJSHandler();

  void AddWorkerObjectToFrame(v8::Local<v8::Context> context, int32_t world_id);

 private:
  bool EnsureConnectedToClient();
  void OnClientDisconnect();

  void CreateWorkerObject(v8::Local<v8::Context> context, int32_t world_id);
  void BindFunctionsToWorkerObject(v8::Isolate* isolate,
                                   int32_t world_id,
                                   v8::Local<v8::Object> worker_object);

  void OnProgress(const std::string& src);
  void OnBlobURL(const std::string& blob_url);
  void OnMediaUpdated(const std::string& src);

  content::RenderFrame* render_frame_ = nullptr;

  mojo::Remote<playlist::mojom::PlaylistRenderFrameBrowserClient> client_;

  base::WeakPtrFactory<PlaylistJSHandler> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_
