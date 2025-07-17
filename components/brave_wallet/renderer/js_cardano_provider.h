/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_PROVIDER_H_

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
// This class implements cardano.brave object.
class JSCardanoProvider final : public gin::Wrappable<JSCardanoProvider>,
                                public content::RenderFrameObserver {
 public:
  ~JSCardanoProvider() override;
  JSCardanoProvider(const JSCardanoProvider&) = delete;
  JSCardanoProvider& operator=(const JSCardanoProvider&) = delete;

  static gin::WrapperInfo kWrapperInfo;

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  static void Install(content::RenderFrame* render_frame);

 private:
  explicit JSCardanoProvider(content::RenderFrame* render_frame);

  bool EnsureConnected();
  v8::Local<v8::Promise> Enable(v8::Isolate* isolate);
  void OnEnableResponse(v8::Global<v8::Context> global_context,
                        v8::Global<v8::Promise::Resolver> promise_resolver,
                        v8::Isolate* isolate,
                        mojom::CardanoProviderErrorBundlePtr error_message);

  v8::Local<v8::Promise> IsEnabled(v8::Isolate* isolate);
  void OnIsEnableResponse(v8::Global<v8::Context> global_context,
                          v8::Global<v8::Promise::Resolver> promise_resolver,
                          v8::Isolate* isolate,
                          const bool enabled);

  std::vector<std::string> GetSupportedExtensions();
  std::string GetName();
  std::string GetIcon();

  void OnDestruct() override;

  mojo::Remote<mojom::CardanoProvider> cardano_provider_;
  base::WeakPtrFactory<JSCardanoProvider> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_CARDANO_PROVIDER_H_
