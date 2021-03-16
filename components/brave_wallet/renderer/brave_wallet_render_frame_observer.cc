/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/brave_wallet_render_frame_observer.h"

#include "base/bind.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_wallet {

BraveWalletRenderFrameObserver::BraveWalletRenderFrameObserver(
    content::RenderFrame* render_frame)
    : RenderFrameObserver(render_frame) {}

BraveWalletRenderFrameObserver::~BraveWalletRenderFrameObserver() {}

void BraveWalletRenderFrameObserver::DidStartNavigation(
    const GURL& url,
    base::Optional<blink::WebNavigationType> navigation_type) {
  url_ = url;
}

void BraveWalletRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!native_javascript_handle_)
    return;

  native_javascript_handle_->AddJavaScriptObjectToFrame(context);
}

void BraveWalletRenderFrameObserver::DidCreateNewDocument() {
  if (!native_javascript_handle_) {
    native_javascript_handle_.reset(
        new BraveWalletJSHandler(render_frame()));
  }
  native_javascript_handle_->InjectScript();
}

void BraveWalletRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_wallet
