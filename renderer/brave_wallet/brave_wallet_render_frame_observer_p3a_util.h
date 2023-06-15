/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_P3A_UTIL_H_
#define BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_P3A_UTIL_H_

#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace v8 {
class Context;
template <class T>
class Local;
class Isolate;
}  // namespace v8

namespace brave_wallet {

class BraveWalletRenderFrameObserverP3AUtil {
 public:
  BraveWalletRenderFrameObserverP3AUtil();
  ~BraveWalletRenderFrameObserverP3AUtil();

  void ReportJSProviders(content::RenderFrame* render_frame,
                         const brave::mojom::DynamicParams& dynamic_params);

 private:
  bool EnsureConnected(content::RenderFrame* render_frame);

  void ReportJSProvider(v8::Isolate* isolate,
                        v8::Local<v8::Context>& context,
                        mojom::CoinType coin_type,
                        const char* provider_object_key,
                        bool allow_provider_overwrite);

  mojo::Remote<brave_wallet::mojom::BraveWalletP3A> brave_wallet_p3a_;
};

}  // namespace brave_wallet

#endif  // BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_P3A_UTIL_H_
