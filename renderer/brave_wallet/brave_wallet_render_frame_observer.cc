/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/components/brave_wallet/common/features.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_wallet {

BraveWalletRenderFrameObserver::BraveWalletRenderFrameObserver(
    content::RenderFrame* render_frame,
    GetDynamicParamsCallback get_dynamic_params_callback)
    : RenderFrameObserver(render_frame),
      get_dynamic_params_callback_(std::move(get_dynamic_params_callback)) {}

BraveWalletRenderFrameObserver::~BraveWalletRenderFrameObserver() {}

void BraveWalletRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    absl::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void BraveWalletRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();
  if (!url_.SchemeIsHTTPOrHTTPS())
    return;
  auto dynamic_params = get_dynamic_params_callback_.Run();
  if (!dynamic_params.brave_use_native_wallet) {
    js_ethereum_provider_.reset();
    js_solana_provider_.reset();
    return;
  }

  if (!js_ethereum_provider_) {
    js_ethereum_provider_.reset(new JSEthereumProvider(
        render_frame(), dynamic_params.brave_use_native_wallet,
        dynamic_params.allow_overwrite_window_web3_provider));
  }
  js_ethereum_provider_->AddJavaScriptObjectToFrame(context);
  js_ethereum_provider_->ConnectEvent();
  js_ethereum_provider_->AllowOverwriteWindowEthereum(
      dynamic_params.allow_overwrite_window_web3_provider);

  if (base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSolanaFeature)) {
    if (!js_solana_provider_) {
      js_solana_provider_ = JSSolanaProvider::Install(
          dynamic_params.brave_use_native_wallet,
          dynamic_params.allow_overwrite_window_web3_provider, render_frame(),
          context);
    } else {
      js_solana_provider_->Init(
          context, dynamic_params.allow_overwrite_window_web3_provider);
    }
  }
}

void BraveWalletRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_wallet
