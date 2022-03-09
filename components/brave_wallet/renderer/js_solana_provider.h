/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_SOLANA_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_SOLANA_PROVIDER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/v8_value_converter.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class JSSolanaProvider final : public gin::Wrappable<JSSolanaProvider> {
 public:
  ~JSSolanaProvider() override;
  JSSolanaProvider(const JSSolanaProvider&) = delete;
  JSSolanaProvider& operator=(const JSSolanaProvider&) = delete;

  static gin::WrapperInfo kWrapperInfo;

  static std::unique_ptr<JSSolanaProvider> Install(
      bool use_native_wallet,
      content::RenderFrame* render_frame,
      v8::Local<v8::Context> context);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  JSSolanaProvider(bool use_native_wallet, content::RenderFrame* render_frame);

  bool EnsureConnected();
  void OnRemoteDisconnect();

  bool GetIsPhantom(gin::Arguments* arguments);
  bool GetIsConnected(gin::Arguments* arguments);
  v8::Local<v8::Promise> Connect(gin::Arguments* arguments);
  v8::Local<v8::Promise> Disconnect(gin::Arguments* arguments);
  v8::Local<v8::Promise> SignAndSendTransaction(gin::Arguments* arguments);
  v8::Local<v8::Promise> SignMessage(gin::Arguments* arguments);
  v8::Local<v8::Promise> Request(gin::Arguments* arguments);
  // Deprecated
  v8::Local<v8::Promise> SignTransaction(gin::Arguments* arguments);
  // Deprecated
  v8::Local<v8::Promise> SignAllTransaction(gin::Arguments* arguments);

  // TODO: fire accountChanged event
  void FireEvent(const std::string& event,
                 std::vector<v8::Local<v8::Value>>&& event_args);

  void OnConnect(v8::Global<v8::Context> global_context,
                 v8::Global<v8::Promise::Resolver> promise_resolver,
                 v8::Isolate* isolate,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 const std::string& public_key);

  void SendResponse(v8::Global<v8::Context> global_context,
                    v8::Global<v8::Promise::Resolver> promise_resolver,
                    v8::Isolate* isolate,
                    v8::Local<v8::Value> response,
                    bool success);

  bool use_native_wallet_ = false;
  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  std::unique_ptr<content::V8ValueConverter> v8_value_converter_;
  mojo::Remote<mojom::SolanaProvider> solana_provider_;
  base::WeakPtrFactory<JSSolanaProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_SOLANA_PROVIDER_H_
