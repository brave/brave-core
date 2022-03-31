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

class JSSolanaProvider final : public gin::Wrappable<JSSolanaProvider>,
                               public mojom::SolanaEventsListener {
 public:
  ~JSSolanaProvider() override;
  JSSolanaProvider(const JSSolanaProvider&) = delete;
  JSSolanaProvider& operator=(const JSSolanaProvider&) = delete;

  static gin::WrapperInfo kWrapperInfo;

  class V8ConverterStrategy : public content::V8ValueConverter::Strategy {
   public:
    V8ConverterStrategy() = default;
    ~V8ConverterStrategy() override = default;

    bool FromV8ArrayBuffer(v8::Local<v8::Object> value,
                           std::unique_ptr<base::Value>* out,
                           v8::Isolate* isolate) override;
  };

  static std::unique_ptr<JSSolanaProvider> Install(
      bool use_native_wallet,
      bool allow_overwrite_window_solana,
      content::RenderFrame* render_frame,
      v8::Local<v8::Context> context);

  bool Init(v8::Local<v8::Context> context, bool allow_overwrite_window_solana);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // mojom::SolanaEventsListener
  void AccountChangedEvent(const absl::optional<std::string>& account) override;

 private:
  JSSolanaProvider(bool use_native_wallet, content::RenderFrame* render_frame);

  void InjectInitScript(bool allow_overwrite_window_solana);
  bool EnsureConnected();
  void OnRemoteDisconnect();

  bool GetIsPhantom(gin::Arguments* arguments);
  bool GetIsConnected(gin::Arguments* arguments);
  // returns solanaWeb3.PublicKey
  v8::Local<v8::Value> GetPublicKey(gin::Arguments* arguments);
  // ({onlyIfTrusted}) => Promise<{ publicKey: solanaWeb3.PublicKey }>
  // {onlyIfTrusted} is optional
  v8::Local<v8::Promise> Connect(gin::Arguments* arguments);
  // () => Promise<undefined>
  v8::Local<v8::Promise> Disconnect(gin::Arguments* arguments);
  // (solanaWeb3.Transaction) => Promise<
  //  { publicKey: <base58 encoded string>,
  //    signature: <base58 encoded string>}>
  v8::Local<v8::Promise> SignAndSendTransaction(gin::Arguments* arguments);
  // (Uint8Array, string display) => Promise<
  //  { publicKey: <solanaWeb3.PublicKey>,
  //    signature: <Uint8Array>}>
  // display encoding is optional
  v8::Local<v8::Promise> SignMessage(gin::Arguments* arguments);
  // It takes { method: <string>, pararms: {...} and return promise accroding to
  // the method:
  // - connect => { publicKey: solanaWeb3.PublicKey}
  // - disconnect => {}
  // - signTransaction => { publicKey: <base58 encoded string>,
  //                        signature: <base58 encoded string>}
  // - signAndSendTransaction => { publicKey: <base58 encoded string>,
  //                               signature: <base58 encoded string>}
  // - signAllTransactions => { publicKey: <base58 encoded string>,
  //                            signature: <base58 encoded string>[]}
  // - signMessage => { publicKey: <base58 encoded string>,
  //                    signature: <base58 encoded string>}
  v8::Local<v8::Promise> Request(gin::Arguments* arguments);
  // Deprecated
  // (solanaWeb3.Transaction) => Promise<solanaWeb3.Transaction>
  v8::Local<v8::Promise> SignTransaction(gin::Arguments* arguments);
  // Deprecated
  // (solanaWeb3.Transaction[]) => Promise<solanaWeb3.Transaction[]>
  v8::Local<v8::Promise> SignAllTransactions(gin::Arguments* arguments);

  void FireEvent(const std::string& event,
                 std::vector<v8::Local<v8::Value>>&& event_args);

  void OnConnect(v8::Global<v8::Context> global_context,
                 v8::Global<v8::Promise::Resolver> promise_resolver,
                 v8::Isolate* isolate,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 const std::string& public_key);

  void OnSignAndSendTransaction(
      v8::Global<v8::Context> global_context,
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      mojom::SolanaProviderError error,
      const std::string& error_message,
      base::Value result);

  void OnSignMessage(v8::Global<v8::Context> global_context,
                     v8::Global<v8::Promise::Resolver> promise_resolver,
                     v8::Isolate* isolate,
                     mojom::SolanaProviderError error,
                     const std::string& error_message,
                     base::Value result);

  void OnSignTransaction(v8::Global<v8::Context> global_context,
                         v8::Global<v8::Promise::Resolver> promise_resolver,
                         v8::Isolate* isolate,
                         mojom::SolanaProviderError error,
                         const std::string& error_message,
                         const std::vector<uint8_t>& serialized_tx);

  void OnSignAllTransactions(
      v8::Global<v8::Context> global_context,
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      mojom::SolanaProviderError error,
      const std::string& error_message,
      const std::vector<std::vector<uint8_t>>& serialized_txs);

  void OnRequest(v8::Global<v8::Context> global_context,
                 v8::Global<v8::Promise::Resolver> promise_resolver,
                 v8::Isolate* isolate,
                 const std::string& method,
                 mojom::SolanaProviderError error,
                 const std::string& error_message,
                 base::Value result);

  void SendResponse(v8::Global<v8::Context> global_context,
                    v8::Global<v8::Promise::Resolver> promise_resolver,
                    v8::Isolate* isolate,
                    v8::Local<v8::Value> response,
                    bool success);

  // Get solanaWeb3.Transaction.serializedMessage with base58 encoding
  absl::optional<std::string> GetSerializedMessage(
      v8::Local<v8::Value> transaction);

  // use @solana/web3.js and create publicKey from base58 string
  v8::Local<v8::Value> CreatePublicKey(v8::Local<v8::Context> context,
                                       const std::string& base58_str);

  // use @solana/web3.js and create Transaction from serialized tx
  v8::Local<v8::Value> CreateTransaction(
      v8::Local<v8::Context> context,
      const std::vector<uint8_t> serialized_tx);

  bool use_native_wallet_ = false;
  raw_ptr<content::RenderFrame> render_frame_ = nullptr;
  std::unique_ptr<content::V8ValueConverter> v8_value_converter_;
  V8ConverterStrategy strategy_;
  mojo::Remote<mojom::SolanaProvider> solana_provider_;
  mojo::Receiver<mojom::SolanaEventsListener> receiver_{this};
  base::WeakPtrFactory<JSSolanaProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_SOLANA_PROVIDER_H_
