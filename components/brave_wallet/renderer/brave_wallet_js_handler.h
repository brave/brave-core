/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "gin/arguments.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/v8.h"

namespace brave_wallet {

class BraveWalletJSHandler : public mojom::EventsListener {
 public:
  explicit BraveWalletJSHandler(content::RenderFrame* render_frame,
                                bool brave_use_native_wallet,
                                bool allow_overwrite_window_ethereum);
  ~BraveWalletJSHandler() override;

  void AddJavaScriptObjectToFrame(v8::Local<v8::Context> context);
  void FireEvent(const std::string& event, base::Value event_args);
  void ConnectEvent();
  void OnGetChainId(const std::string& chain_id);
  void DisconnectEvent(const std::string& message);
  void AccountsChangedEvent(const std::vector<std::string>& accounts) override;
  void ChainChangedEvent(const std::string& chain_id) override;
  void AllowOverwriteWindowEthereum(bool allow);

 private:
  void BindFunctionsToObject(v8::Isolate* isolate,
                             v8::Local<v8::Context> context,
                             v8::Local<v8::Object> ethereum_object,
                             v8::Local<v8::Object> metamask_object);
  void UpdateAndBindJSProperties();
  void UpdateAndBindJSProperties(v8::Isolate* isolate,
                                 v8::Local<v8::Context> context,
                                 v8::Local<v8::Object> ethereum_obj);

  // Adds a function to the provided object.
  template <typename Sig>
  void BindFunctionToObject(v8::Isolate* isolate,
                            v8::Local<v8::Object> javascript_object,
                            const std::string& name,
                            const base::RepeatingCallback<Sig>& callback);
  void CreateEthereumObject(v8::Isolate* isolate,
                            v8::Local<v8::Context> context);
  bool EnsureConnected();
  void OnRemoteDisconnect();
  void InjectInitScript();
  void ExecuteScript(const std::string script);

  // Functions to be called from JS
  v8::Local<v8::Promise> Request(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input);
  v8::Local<v8::Value> IsConnected();
  v8::Local<v8::Promise> Enable();
  v8::Local<v8::Promise> IsUnlocked();
  v8::Local<v8::Promise> Send(gin::Arguments* args);
  void SendAsync(gin::Arguments* args);

  void OnRequestOrSendAsync(v8::Global<v8::Context> global_context,
                            std::unique_ptr<v8::Global<v8::Function>> callback,
                            v8::Global<v8::Promise::Resolver> promise_resolver,
                            v8::Isolate* isolate,
                            bool force_json_response,
                            base::Value id,
                            base::Value formed_response,
                            const bool reject,
                            const std::string& first_allowed_account,
                            const bool update_bind_js_properties);

  void OnIsUnlocked(v8::Global<v8::Context> global_context,
                    v8::Global<v8::Promise::Resolver> promise_resolver,
                    v8::Isolate* isolate,
                    bool locked);
  void SendResponse(base::Value id,
                    v8::Global<v8::Context> global_context,
                    std::unique_ptr<v8::Global<v8::Function>> callback,
                    v8::Global<v8::Promise::Resolver> promise_resolver,
                    v8::Isolate* isolate,
                    bool force_json_response,
                    std::unique_ptr<base::Value> formed_response,
                    bool success);

  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  bool brave_use_native_wallet_;
  bool allow_overwrite_window_ethereum_;
  mojo::Remote<mojom::BraveWalletProvider> brave_wallet_provider_;
  mojo::Receiver<mojom::EventsListener> receiver_{this};
  bool is_connected_;
  std::string chain_id_;
  std::string first_allowed_account_;
  base::WeakPtrFactory<BraveWalletJSHandler> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_BRAVE_WALLET_JS_HANDLER_H_
