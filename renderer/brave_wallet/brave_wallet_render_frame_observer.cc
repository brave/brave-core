/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "build/buildflag.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
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
    std::optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

bool BraveWalletRenderFrameObserver::IsPageValid() {
  // There could be empty, invalid and "about:blank" URLs,
  // they should fallback to the main frame rules
  if (url_.is_empty() || !url_.is_valid() || url_.spec() == "about:blank") {
    url_ = url::Origin(render_frame()->GetWebFrame()->GetSecurityOrigin())
               .GetURL();
  }
  return url_.SchemeIsHTTPOrHTTPS();
}

bool BraveWalletRenderFrameObserver::CanCreateProvider() {
  if (!IsPageValid()) {
    return false;
  }

  // Wallet provider objects should only be created in secure contexts
  if (!render_frame()->GetWebFrame()->GetDocument().IsSecureContext()) {
    return false;
  }

  // Scripts can't be executed on provisional frames
  if (render_frame()->GetWebFrame()->IsProvisional()) {
    return false;
  }

  return true;
}

void BraveWalletRenderFrameObserver::DidFinishLoad() {
#if !BUILDFLAG(IS_ANDROID)
  // Only record P3A for desktop and valid HTTP/HTTPS pages
  if (!IsPageValid()) {
    return;
  }

  auto dynamic_params = get_dynamic_params_callback_.Run();

  p3a_util_.ReportJSProviders(render_frame(), dynamic_params);
#endif
}

void BraveWalletRenderFrameObserver::DidClearWindowObject() {
  if (!CanCreateProvider()) {
    return;
  }

  CHECK(render_frame());
  v8::Isolate* isolate =
      render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  auto* web_frame = render_frame()->GetWebFrame();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  auto dynamic_params = get_dynamic_params_callback_.Run();
  if (!dynamic_params.install_window_brave_ethereum_provider &&
      !dynamic_params.install_window_ethereum_provider &&
      !dynamic_params.brave_use_native_solana_wallet) {
    return;
  }

  if (!dynamic_params.install_window_brave_ethereum_provider &&
      dynamic_params.install_window_ethereum_provider) {
    NOTREACHED_IN_MIGRATION();
    return;
  }

  if (dynamic_params.install_window_brave_ethereum_provider &&
      web_frame->GetDocument().IsDOMFeaturePolicyEnabled(isolate, context,
                                                         "ethereum")) {
    JSEthereumProvider::Install(
        dynamic_params.install_window_ethereum_provider,
        dynamic_params.allow_overwrite_window_ethereum_provider,
        render_frame());
  }

  if (web_frame->GetDocument().IsDOMFeaturePolicyEnabled(isolate, context,
                                                         "solana") &&
      dynamic_params.brave_use_native_solana_wallet) {
    JSSolanaProvider::Install(
        dynamic_params.allow_overwrite_window_solana_provider, render_frame());
  }
}

void BraveWalletRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_wallet
