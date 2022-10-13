/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "brave/components/brave_wallet/common/features.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_wallet {

BraveWalletRenderFrameObserver::BraveWalletRenderFrameObserver(
    content::RenderFrame* render_frame,
    GetDynamicParamsCallback get_dynamic_params_callback)
    : RenderFrameObserver(render_frame),
      get_dynamic_params_callback_(std::move(get_dynamic_params_callback)) {}

BraveWalletRenderFrameObserver::~BraveWalletRenderFrameObserver() = default;

void BraveWalletRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    absl::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

bool BraveWalletRenderFrameObserver::CanCreateProvider() {
  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank")
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();
  if (!url_.SchemeIsHTTPOrHTTPS())
    return false;

  // Wallet provider objects should only be created in secure contexts
  if (!render_frame()->GetWebFrame()->GetDocument().IsSecureContext()) {
    return false;
  }

  return true;
}

void BraveWalletRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!CanCreateProvider())
    return;

  auto dynamic_params = get_dynamic_params_callback_.Run();
  if (!dynamic_params.brave_use_native_ethereum_wallet) {
    js_ethereum_provider_.reset();
    return;
  }

  bool is_main_world = world_id == content::ISOLATED_WORLD_ID_GLOBAL;
  if (render_frame()->GetWebFrame()->GetDocument().IsDOMFeaturePolicyEnabled(
          render_frame()->GetWebFrame()->MainWorldScriptContext(),
          "ethereum")) {
    if (!js_ethereum_provider_) {
      js_ethereum_provider_ = std::make_unique<JSEthereumProvider>(
          render_frame(), dynamic_params.brave_use_native_ethereum_wallet);
    }
    js_ethereum_provider_->AddJavaScriptObjectToFrame(
        context, dynamic_params.allow_overwrite_window_ethereum_provider,
        is_main_world);
    js_ethereum_provider_->ConnectEvent();
  }
}

void BraveWalletRenderFrameObserver::WillReleaseScriptContext(
    v8::Local<v8::Context>,
    int32_t world_id) {
  js_ethereum_provider_.reset();
}

void BraveWalletRenderFrameObserver::DidClearWindowObject() {
  if (!CanCreateProvider())
    return;

  auto dynamic_params = get_dynamic_params_callback_.Run();
  if (!dynamic_params.brave_use_native_solana_wallet) {
    return;
  }

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::HandleScope handle_scope(isolate);
  auto* web_frame = render_frame()->GetWebFrame();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  if (base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSolanaFeature) &&
      base::FeatureList::IsEnabled(
          brave_wallet::features::kBraveWalletSolanaProviderFeature) &&
      web_frame->GetDocument().IsDOMFeaturePolicyEnabled(context, "solana")) {
    JSSolanaProvider::Install(
        dynamic_params.allow_overwrite_window_solana_provider, render_frame());
  }
}

void BraveWalletRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_wallet
