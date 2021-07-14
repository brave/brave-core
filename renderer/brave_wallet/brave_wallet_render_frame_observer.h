/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_
#define BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "brave/common/brave_renderer_configuration.mojom.h"
#include "brave/components/brave_wallet/renderer/brave_wallet_js_handler.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace brave_wallet {

class BraveWalletRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit BraveWalletRenderFrameObserver(
      content::RenderFrame* render_frame,
      brave::mojom::DynamicParams dynamic_params);
  ~BraveWalletRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidStartNavigation(
      const GURL& url,
      absl::optional<blink::WebNavigationType> navigation_type) override;
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<BraveWalletJSHandler> native_javascript_handle_;

  GURL url_;
  const brave::mojom::DynamicParams dynamic_params_;
};

}  // namespace brave_wallet

#endif  // BRAVE_RENDERER_BRAVE_WALLET_BRAVE_WALLET_RENDER_FRAME_OBSERVER_H_
