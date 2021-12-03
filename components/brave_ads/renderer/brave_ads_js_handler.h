/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_JS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_JS_HANDLER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/brave_ads_host.mojom.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace content {
class RenderFrame;
}

namespace brave_ads {

class BraveAdsJSHandler final {
 public:
  explicit BraveAdsJSHandler(content::RenderFrame* render_frame);
  BraveAdsJSHandler(const BraveAdsJSHandler&) = delete;
  BraveAdsJSHandler& operator=(const BraveAdsJSHandler&) = delete;
  ~BraveAdsJSHandler();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);

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
  void OnRemoteDisconnect();

  // A function to be called from JS
  v8::Local<v8::Promise> RequestAdsEnabled(v8::Isolate* isolate);
  void OnRequestAdsEnabled(
      std::unique_ptr<v8::Global<v8::Promise::Resolver>> promise_resolver,
      v8::Isolate* isolate,
      std::unique_ptr<v8::Global<v8::Context>> context_old,
      bool response);

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  mojo::Remote<brave_ads::mojom::BraveAdsHost> brave_ads_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_RENDERER_BRAVE_ADS_JS_HANDLER_H_
