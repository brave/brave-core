/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_WALLET_API_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_WALLET_API_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// https://cips.cardano.org/cip/CIP-30
// This class implements API object which is available after
// cardano.brave.enable() is called.
class JSCardanoWalletApi final
    : public gin::DeprecatedWrappable<JSCardanoWalletApi>,
      public content::RenderFrameObserver {
 public:
  JSCardanoWalletApi(base::PassKey<class JSCardanoProvider> pass_key,
                     v8::Local<v8::Context> context,
                     v8::Isolate* isolate,
                     content::RenderFrame* render_frame);
  ~JSCardanoWalletApi() override;
  JSCardanoWalletApi(const JSCardanoWalletApi&) = delete;
  JSCardanoWalletApi& operator=(const JSCardanoWalletApi&) = delete;

  static gin::DeprecatedWrapperInfo kWrapperInfo;

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 private:
  bool EnsureConnected();
  void OnDestruct() override;

  void HandleStringResult(v8::Global<v8::Context> global_context,
                          v8::Global<v8::Promise::Resolver> promise_resolver,
                          v8::Isolate* isolate,
                          const std::string& result,
                          const std::optional<std::string>& error);
  void HandleStringVecResult(v8::Global<v8::Context> global_context,
                             v8::Global<v8::Promise::Resolver> promise_resolver,
                             v8::Isolate* isolate,
                             const std::vector<std::string>& result,
                             const std::optional<std::string>& error);
  void HandleUtxoVecResult(
      v8::Global<v8::Context> global_context,
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      const std::optional<std::vector<std::string>>& result,
      const std::optional<std::string>& error_message);

  v8::Local<v8::Promise> GetNetworkId(v8::Isolate* isolate);
  void OnGetNetworkId(v8::Global<v8::Context> global_context,
                      v8::Global<v8::Promise::Resolver> promise_resolver,
                      v8::Isolate* isolate,
                      int32_t network,
                      const std::optional<std::string>& error_message);

  v8::Local<v8::Promise> GetUsedAddresses(v8::Isolate* isolate);

  v8::Local<v8::Promise> GetUnusedAddresses(v8::Isolate* isolate);

  v8::Local<v8::Promise> GetChangeAddress(v8::Isolate* isolate);

  v8::Local<v8::Promise> GetRewardAddresses(v8::Isolate* isolate);

  v8::Local<v8::Promise> GetBalance(v8::Isolate* isolate);

  v8::Local<v8::Promise> GetUtxos(gin::Arguments* args);

  v8::Local<v8::Promise> SignTx(gin::Arguments* args);

  v8::Local<v8::Promise> SignData(gin::Arguments* args);
  void OnSignData(v8::Global<v8::Context> global_context,
                  v8::Global<v8::Promise::Resolver> promise_resolver,
                  v8::Isolate* isolate,
                  mojom::CardanoProviderSignatureResultPtr result,
                  const std::optional<std::string>& error_message);

  v8::Local<v8::Promise> SubmitTx(gin::Arguments* args);

  v8::Local<v8::Promise> GetExtensions(gin::Arguments* args);

  v8::Local<v8::Promise> GetCollateral(gin::Arguments* args);

  mojo::Remote<mojom::CardanoProvider> cardano_provider_;
  base::WeakPtrFactory<JSCardanoWalletApi> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_WALLET_API_H_
