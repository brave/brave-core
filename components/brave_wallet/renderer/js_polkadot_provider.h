/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/arguments.h"
#include "gin/public/wrappable_pointer_tags.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// https://github.com/polkadot-js/extension#injection-information
// This class implements the `window.injectedWeb3['brave-wallet']` object:
// `version`, plus the `enable()` and `connect()` hooks that resolve to the
// `Injected` API. See JSPolkadotInjected for what they resolve to.
class JSPolkadotProvider final : public gin::Wrappable<JSPolkadotProvider>,
                                 public content::RenderFrameObserver {
 public:
  explicit JSPolkadotProvider(content::RenderFrame* render_frame);
  ~JSPolkadotProvider() override;
  JSPolkadotProvider(const JSPolkadotProvider&) = delete;
  JSPolkadotProvider& operator=(const JSPolkadotProvider&) = delete;

  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kPolkadotProvider};
  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;

  static void Install(content::RenderFrame* render_frame);

 private:
  bool EnsureConnected();

  std::string GetVersion();

  static JSPolkadotProvider* ProviderFromCallbackData(
      const v8::FunctionCallbackInfo<v8::Value>& info);

  v8::Local<v8::Promise> Enable(v8::Isolate* isolate);
  v8::Local<v8::Promise> Connect(v8::Isolate* isolate);

  v8::Local<v8::Function> GetEnableObject(v8::Isolate* isolate);
  static void EnableImpl(const v8::FunctionCallbackInfo<v8::Value>& info);

  v8::Local<v8::Function> GetConnectObject(v8::Isolate* isolate);
  static void ConnectImpl(const v8::FunctionCallbackInfo<v8::Value>& info);

  // The with_extension_info name comes from the:
  //     export type InjectedExtension = InjectedExtensionInfo & Injected;
  // type definition in the @polkadot/extension-inject module.
  v8::Local<v8::Promise> RequestPermission(v8::Isolate* isolate,
                                           bool with_extension_info);
  void OnEnableResponse(v8::Global<v8::Context> global_context,
                        v8::Global<v8::Promise::Resolver> promise_resolver,
                        v8::Isolate* isolate,
                        bool with_extension_info,
                        mojo::PendingRemote<mojom::PolkadotApi> pending_remote,
                        mojom::PolkadotProviderErrorBundlePtr error);

  // content::RenderFrameObserver
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int32_t world_id) override;
  void OnDestruct() override;
  void Cleanup();

  mojo::Remote<mojom::PolkadotProvider> polkadot_provider_;
  base::WeakPtrFactory<JSPolkadotProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_
