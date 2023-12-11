/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_
#define BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/platform/web_string.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}  // namespace content

namespace playlist {

class PlaylistJSHandler {
 public:
  PlaylistJSHandler(content::RenderFrame* render_frame,
                    const int32_t isolated_world_id);
  ~PlaylistJSHandler();

  void AddWorkerObjectToFrame(v8::Local<v8::Context> context);

  void SetDetectorScript(const blink::WebString& script);
  void allow_to_run_script_on_main_world() {
    allow_to_run_script_on_main_world_ = true;
  }

 private:
  bool EnsureConnectedToMediaHandler();
  void OnMediaHandlerDisconnect();

  void CreateWorkerObject(v8::Isolate* isolate, v8::Local<v8::Context> context);
  void BindFunctionsToWorkerObject(v8::Isolate* isolate,
                                   v8::Local<v8::Object> worker_object);

  void OnMediaUpdated(const std::string& src);
  void OnFindMedia(GURL requested_url,
                   absl::optional<base::Value> value,
                   base::TimeTicks time_ticks);

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;

  const int32_t isolated_world_id_;

  blink::WebString script_;

  mojo::Remote<playlist::mojom::PlaylistMediaHandler> media_handler_;

  bool allow_to_run_script_on_main_world_ = false;

  base::WeakPtrFactory<PlaylistJSHandler> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_RENDERER_PLAYLIST_JS_HANDLER_H_
