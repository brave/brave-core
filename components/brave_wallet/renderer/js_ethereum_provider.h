/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_ETHEREUM_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_ETHEREUM_PROVIDER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-forward.h"

namespace brave_wallet {

class JSEthereumProvider final : public gin::Wrappable<JSEthereumProvider>,
                                 public content::RenderFrameObserver,
                                 public mojom::EventsListener {
 public:
  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kEthereumProvider};

  explicit JSEthereumProvider(content::RenderFrame* render_frame);
  ~JSEthereumProvider() override;
  JSEthereumProvider(const JSEthereumProvider&) = delete;
  JSEthereumProvider& operator=(const JSEthereumProvider&) = delete;

  static void Install(bool install_ethereum_provider,
                      bool allow_overwrite_window_ethereum_provider,
                      content::RenderFrame* render_frame);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;

  // mojom::EventsListener
  void AccountsChangedEvent(const std::vector<std::string>& accounts) override;
  void ChainChangedEvent(const std::string& chain_id) override;
  void MessageEvent(const std::string& subscription_id,
                    base::Value result) override;

 private:
  class MetaMask final : public gin::Wrappable<MetaMask> {
   public:
    static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                      gin::kMetaMask};

    explicit MetaMask(content::RenderFrame*);
    ~MetaMask() override;
    MetaMask(const MetaMask&) = delete;
    MetaMask& operator=(const MetaMask&) = delete;

    // gin::WrappableBase
    gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
        v8::Isolate* isolate) override;
    const gin::WrapperInfo* wrapper_info() const override;
    v8::Local<v8::Promise> IsUnlocked(v8::Isolate* isolate);

   private:
    void OnIsUnlocked(v8::Global<v8::Context> global_context,
                      v8::Global<v8::Promise::Resolver> promise_resolver,
                      v8::Isolate* isolate,
                      bool locked);
    raw_ptr<content::RenderFrame> render_frame_;
    mojo::Remote<mojom::EthereumProvider> ethereum_provider_;
  };

  // content::RenderFrameObserver
  void OnDestruct() override;
  void WillReleaseScriptContext(v8::Local<v8::Context>,
                                int32_t world_id) override;
  void DidDispatchDOMContentLoadedEvent() override;
  void DidFinishLoad() override;

  bool EnsureConnected();

  void FireEvent(const std::string& event, base::ValueView event_args);
  void OnGetChainId(const std::string& chain_id);
  void ConnectEvent();
  void DisconnectEvent(const std::string& message);

  bool GetIsBraveWallet();
  bool GetIsMetaMask();
  v8::Local<v8::Value> GetMetaMask(v8::Isolate* isolate);
  std::string GetChainId();
  v8::Local<v8::Value> GetNetworkVersion(v8::Isolate* isolate);
  v8::Local<v8::Value> GetSelectedAddress(v8::Isolate* isolate);

  // Functions to be called from JS
  v8::Local<v8::Promise> Request(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input);
  bool IsConnected();
  v8::Local<v8::Promise> Enable(v8::Isolate* isolate);
  v8::Local<v8::Promise> SendMethod(gin::Arguments* args);
  void SendAsync(gin::Arguments* args);

  void OnRequestOrSendAsync(v8::Global<v8::Context> global_context,
                            std::unique_ptr<v8::Global<v8::Function>> callback,
                            v8::Global<v8::Promise::Resolver> promise_resolver,
                            v8::Isolate* isolate,
                            mojom::EthereumProviderResponsePtr response);

  void SendResponse(base::Value id,
                    v8::Global<v8::Context> global_context,
                    std::unique_ptr<v8::Global<v8::Function>> callback,
                    v8::Global<v8::Promise::Resolver> promise_resolver,
                    v8::Isolate* isolate,
                    base::Value formed_response,
                    bool success);

  void OnProviderRequested();
  void BindRequestProviderListener();
  void OnRequestProvider();
  void AnnounceProvider();
  const std::string& GetBraveWalletImage();

  mojo::Remote<mojom::EthereumProvider> ethereum_provider_;
  mojo::Receiver<mojom::EventsListener> receiver_{this};
  bool is_connected_ = false;
  bool script_context_released_ = false;
  std::string chain_id_;
  std::string first_allowed_account_;
  std::string uuid_;
  std::optional<std::string> brave_wallet_image_;

  // Persistent self-reference to prevent GC from freeing this object while
  // it's still needed for JavaScript bindings. Cleared in OnDestruct().
  cppgc::Persistent<JSEthereumProvider> self_;

  base::WeakPtrFactory<JSEthereumProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_ETHEREUM_PROVIDER_H_
