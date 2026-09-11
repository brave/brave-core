/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_INJECTED_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_INJECTED_H_

#include <optional>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/pass_key.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/public/wrappable_pointer_tags.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

// Backs the `Injected` object that `enable()` resolves to. The `Injected`
// shape is nested (`accounts.get()`, `signer.signPayload()`) while gin methods
// must be called with their own wrapper as the receiver, so this class stays
// flat and a shim script assembles the nested object around it. See
// `kPolkadotInjectedScript` in js_polkadot_provider.cc.
// https://github.com/polkadot-js/extension/blob/master/packages/extension-inject/src/types.ts
class JSPolkadotInjected final : public gin::Wrappable<JSPolkadotInjected>,
                                 public content::RenderFrameObserver {
 public:
  JSPolkadotInjected(mojo::Remote<mojom::PolkadotApi> remote,
                     base::PassKey<class JSPolkadotProvider> pass_key,
                     content::RenderFrame* render_frame);
  ~JSPolkadotInjected() override;
  JSPolkadotInjected(const JSPolkadotInjected&) = delete;
  JSPolkadotInjected& operator=(const JSPolkadotInjected&) = delete;

  static constexpr gin::WrapperInfo kWrapperInfo = {{gin::kEmbedderNativeGin},
                                                    gin::kPolkadotInjected};

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;

 private:
  // `accounts.get(anyType)`
  v8::Local<v8::Promise> GetAccounts(v8::Isolate* isolate, bool any_type);
  void OnGetAccountsResponse(
      v8::Global<v8::Context> global_context,
      v8::Global<v8::Promise::Resolver> promise_resolver,
      v8::Isolate* isolate,
      std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>> accounts,
      mojom::PolkadotProviderErrorBundlePtr error);

  // `signer.signPayload(payload)` and `signer.signRaw(payload)`. Not
  // implemented yet; both reject.
  v8::Local<v8::Promise> SignPayload(v8::Isolate* isolate,
                                     v8::Local<v8::Value> payload);
  v8::Local<v8::Promise> SignRaw(v8::Isolate* isolate,
                                 v8::Local<v8::Value> payload);

  void Cleanup();

  // content::RenderFrameObserver
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int32_t world_id) override;
  void OnDestruct() override;

  mojo::Remote<mojom::PolkadotApi> polkadot_api_;

  base::WeakPtrFactory<JSPolkadotInjected> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_INJECTED_H_
