/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_ADVERTISE_JS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_ADVERTISE_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_talk/common/brave_talk_advertise.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_talk {

class BraveTalkFrameJSHandler {
 public:
  explicit BraveTalkFrameJSHandler(content::RenderFrame* render_frame);
  BraveTalkFrameJSHandler(const BraveTalkFrameJSHandler&) = delete;
  BraveTalkFrameJSHandler& operator=(const BraveTalkFrameJSHandler&) =
      delete;
  ~BraveTalkFrameJSHandler();

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
  v8::Local<v8::Promise> GetCanSetDefaultSearchProvider(v8::Isolate* isolate);
  void SetIsDefaultSearchProvider(v8::Isolate* isolate);

  void OnDeviceIdReceived(
      std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
      v8::Isolate* isolate,
      std::unique_ptr<v8::Global<v8::Context>> context_old,
      const std::string& result);

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<brave_talk::mojom::BraveTalkAdvertise> brave_talk_advertise_;
};

}  // namespace brave_search

#endif // BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_ADVERTISE_JS_HANDLER_H_
