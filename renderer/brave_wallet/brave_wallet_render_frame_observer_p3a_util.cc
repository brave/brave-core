/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_wallet/brave_wallet_render_frame_observer_p3a_util.h"

#include "brave/components/brave_wallet/renderer/v8_helper.h"
#include "gin/converter.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8.h"

namespace brave_wallet {

BraveWalletRenderFrameObserverP3AUtil::BraveWalletRenderFrameObserverP3AUtil() {
}
BraveWalletRenderFrameObserverP3AUtil::
    ~BraveWalletRenderFrameObserverP3AUtil() {}

void BraveWalletRenderFrameObserverP3AUtil::ReportEthereumProvider(
    content::RenderFrame* render_frame,
    const brave::mojom::DynamicParams& dynamic_params) {
  if (!EnsureConnected(render_frame)) {
    return;
  }

  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  auto* web_frame = render_frame->GetWebFrame();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  if (context.IsEmpty()) {
    return;
  }
  v8::MicrotasksScope microtasks(isolate, context->GetMicrotaskQueue(),
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Value> ethereum_value;
  v8::Local<v8::Object> ethereum_obj;
  mojom::EthereumProviderType ethereum_provider_type =
      mojom::EthereumProviderType::None;
  if (context->Global()
          ->Get(context, gin::StringToV8(isolate, "ethereum"))
          .ToLocal(&ethereum_value) &&
      ethereum_value->IsObject() &&
      ethereum_value->ToObject(context).ToLocal(&ethereum_obj)) {
    bool strict_native_wallet_enabled =
        dynamic_params.brave_use_native_ethereum_wallet &&
        !dynamic_params.allow_overwrite_window_ethereum_provider;
    v8::Local<v8::Value> is_brave_wallet;

    if ((GetProperty(context, ethereum_obj, "isBraveWallet")
             .ToLocal(&is_brave_wallet) &&
         is_brave_wallet->BooleanValue(isolate)) ||
        strict_native_wallet_enabled) {
      ethereum_provider_type = mojom::EthereumProviderType::Native;
    } else {
      ethereum_provider_type = mojom::EthereumProviderType::ThirdParty;
    }
  }
  brave_wallet_p3a_->ReportEthereumProvider(ethereum_provider_type);
}

bool BraveWalletRenderFrameObserverP3AUtil::EnsureConnected(
    content::RenderFrame* render_frame) {
  if (!brave_wallet_p3a_.is_bound()) {
    render_frame->GetBrowserInterfaceBroker()->GetInterface(
        brave_wallet_p3a_.BindNewPipeAndPassReceiver());
  }
  return brave_wallet_p3a_.is_bound();
}

}  // namespace brave_wallet
