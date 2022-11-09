/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_
#define BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/renderer/js_ethereum_provider.h"
#include "brave/components/brave_wallet/renderer/js_solana_provider.h"
#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer_p3a_util.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_wallet {

class BraveWalletRenderFrameObserver : public content::RenderFrameObserver {
 public:
  using GetDynamicParamsCallback =
      base::RepeatingCallback<const brave::mojom::DynamicParams&()>;

  explicit BraveWalletRenderFrameObserver(
      content::RenderFrame* render_frame,
      GetDynamicParamsCallback get_dynamic_params_callback);
  ~BraveWalletRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      absl::optional<blink::WebNavigationType> navigation_type) override;
  void DidClearWindowObject() override;

  void DidFinishLoad() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  bool IsPageValid();
  bool CanCreateProvider();

  GURL url_;
  GetDynamicParamsCallback get_dynamic_params_callback_;

  BraveWalletRenderFrameObserverP3AUtil p3a_util_;
};

}  // namespace brave_wallet

#endif  // BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_
