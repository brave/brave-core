/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_wallet {

class BraveWalletJSHandler {
 public:
  explicit BraveWalletJSHandler(content::RenderFrame* render_frame);
  ~BraveWalletJSHandler();

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  void InjectScript();

 private:
  void CreateWorkerObject(v8::Isolate* isolate, v8::Local<v8::Context> context);
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Object> javascript_object);
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  bool EnsureConnected();

  // Functions to be called from JS
  void Request(const std::string& input);

  void OnRequest(const int status, const std::string& response);

  content::RenderFrame* render_frame_;
  mojo::Remote<mojom::BraveWalletProvider> brave_wallet_provider_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_
