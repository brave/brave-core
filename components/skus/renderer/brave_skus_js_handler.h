/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_rewards {

class BraveSkusJSHandler {
 public:
  explicit BraveSkusJSHandler(content::RenderFrame* render_frame);
  BraveSkusJSHandler(const BraveSkusJSHandler&) = delete;
  BraveSkusJSHandler& operator=(const BraveSkusJSHandler&) =
      delete;
  ~BraveSkusJSHandler();

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
  v8::Local<v8::Promise> RefreshOrder(v8::Isolate* isolate,
                                      std::string order_id);
  void OnRefreshOrder(v8::Global<v8::Promise::Resolver> promise_resolver,
                      v8::Isolate* isolate,
                      v8::Global<v8::Context> context_old,
                      const std::string& response);

  content::RenderFrame* render_frame_;
  mojo::Remote<skus::mojom::SkusSdk> skus_sdk_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_RENDERER_BRAVE_SKUS_JS_HANDLER_H_
