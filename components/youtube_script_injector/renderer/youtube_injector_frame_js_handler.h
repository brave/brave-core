// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_INJECTOR_FRAME_JS_HANDLER_H_
#define BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_INJECTOR_FRAME_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/youtube_script_injector/common/youtube_injector.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace youtube_script_injector {

class YouTubeInjectorFrameJSHandler {
 public:
  YouTubeInjectorFrameJSHandler(content::RenderFrame* render_frame);
  YouTubeInjectorFrameJSHandler(const YouTubeInjectorFrameJSHandler&) = delete;
  YouTubeInjectorFrameJSHandler& operator=(const YouTubeInjectorFrameJSHandler&) =
      delete;
  ~YouTubeInjectorFrameJSHandler();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  void ResetRemote(content::RenderFrame* render_frame);

 private:
  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context);
  bool EnsureConnected();

  // A function to be called from JS
  void NativePipMode();

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<youtube_script_injector::mojom::YouTubeInjector> youtube_injector_;
};

}  // namespace youtube_script_injector

#endif  // BRAVE_COMPONENTS_YOUTUBE_SCRIPT_INJECTOR_RENDERER_YOUTUBE_INJECTOR_FRAME_JS_HANDLER_H_
