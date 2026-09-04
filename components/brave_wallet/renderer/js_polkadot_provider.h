/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_

#include <string>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/public/wrappable_pointer_tags.h"
#include "gin/wrappable.h"

namespace brave_wallet {

// https://github.com/polkadot-js/extension#injection-information
// This class implements the `window.injectedWeb3['brave-wallet']` object. Only
// `version` is exposed so far; `enable()` and the `Injected` API it resolves to
// are not implemented yet.
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
  std::string GetVersion();

  // content::RenderFrameObserver
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int32_t world_id) override;
  void OnDestruct() override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_JS_POLKADOT_PROVIDER_H_
