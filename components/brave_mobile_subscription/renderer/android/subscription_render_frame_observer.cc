// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_mobile_subscription/renderer/android/subscription_render_frame_observer.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/skus/renderer/skus_utils.h"
#include "build/build_config.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_util.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/features.h"
#endif

namespace brave_subscription {

namespace {

inline constexpr char kIntentParamName[] = "intent";
inline constexpr char kIntentParamValue[] = "connect-receipt";
inline constexpr char kIntentParamTestValue[] = "connect-receipt-test";
inline constexpr char kProductParamName[] = "product";
inline constexpr char kProductVPNParamValue[] = "vpn";
inline constexpr char kProductLeoParamValue[] = "leo";
inline constexpr char kIntentParamValueLeo[] = "link-order";

}  // namespace

SubscriptionRenderFrameObserver::SubscriptionRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

SubscriptionRenderFrameObserver::~SubscriptionRenderFrameObserver() = default;

bool SubscriptionRenderFrameObserver::EnsureConnected() {
  bool bound = false;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  if (brave_vpn::IsBraveVPNFeatureEnabled() && product_ == Product::kVPN) {
    if (!vpn_service_.is_bound()) {
      render_frame()->GetBrowserInterfaceBroker().GetInterface(
          vpn_service_.BindNewPipeAndPassReceiver());
    }
    bound |= vpn_service_.is_bound();
  }
#endif
#if BUILDFLAG(ENABLE_AI_CHAT)
  if (ai_chat::features::IsAIChatEnabled() && product_ == Product::kLeo) {
    if (!ai_chat_subscription_.is_bound()) {
      render_frame()->GetBrowserInterfaceBroker().GetInterface(
          ai_chat_subscription_.BindNewPipeAndPassReceiver());
    }
    bound |= ai_chat_subscription_.is_bound();
  }
#endif
  return bound;
}

void SubscriptionRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id) {
    return;
  }

  if (!IsAllowed()) {
    return;
  }

  DCHECK(product_.has_value());
  if (!product_.has_value()) {
    return;
  }
  auto connected = EnsureConnected();
  if (!connected) {
    LOG(ERROR) << "Failed to establish connection to a mojo channel";
    return;
  }

  if (product_ == Product::kVPN) {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    if (vpn_service_.is_bound()) {
      vpn_service_->GetPurchaseToken(
          base::BindOnce(&SubscriptionRenderFrameObserver::OnGetPurchaseToken,
                         weak_factory_.GetWeakPtr()));
    }
#endif
  } else if (product_ == Product::kLeo) {
#if BUILDFLAG(ENABLE_AI_CHAT)
    if (ai_chat_subscription_.is_bound()) {
      ai_chat_subscription_->GetPurchaseTokenOrderId(base::BindOnce(
          &SubscriptionRenderFrameObserver::OnGetPurchaseTokenOrderId,
          weak_factory_.GetWeakPtr()));
    }
#endif
  }
}

std::string SubscriptionRenderFrameObserver::GetPurchaseTokenJSString(
    const std::string& purchase_token) {
  if (!IsValueAllowed(purchase_token)) {
    return "";
  }

  std::string_view receipt_var_name;
  if (product_ == Product::kVPN) {
    receipt_var_name = "braveVpn.receipt";
  } else if (product_ == Product::kLeo) {
    receipt_var_name = "braveLeo.receipt";
  }

  return base::StrCat({"window.localStorage.setItem(\"", receipt_var_name,
                       "\", \"", purchase_token, "\");"});
}

void SubscriptionRenderFrameObserver::OnGetPurchaseToken(
    const std::string& purchase_token) {
  if (!IsAllowed()) {
    return;
  }
  auto* frame = render_frame();
  if (frame) {
    std::string set_local_storage = GetPurchaseTokenJSString(purchase_token);
    if (!set_local_storage.empty()) {
      frame->ExecuteJavaScript(base::UTF8ToUTF16(set_local_storage));
    }
  }
}

void SubscriptionRenderFrameObserver::OnGetPurchaseTokenOrderId(
    const std::string& purchase_token,
    const std::string& order_id) {
  if (!IsAllowed()) {
    return;
  }
  auto* frame = render_frame();
  if (frame && !order_id.empty() && !purchase_token.empty()) {
    frame->ExecuteJavaScript(base::UTF8ToUTF16(base::StrCat(
        {"window.localStorage.setItem(\"braveLeo.orderId\", \"", order_id,
         "\");", GetPurchaseTokenJSString(purchase_token)})));
  }
}

std::string SubscriptionRenderFrameObserver::ExtractParam(
    const GURL& url,
    const std::string& name) const {
  url::Component query(0, static_cast<int>(url.query_piece().length())), key,
      value;
  std::string_view data =
      url.query_piece().data() ? url.query_piece().data() : "";
  while (url::ExtractQueryKeyValue(data, &query, &key, &value)) {
    std::string_view key_str = url.query_piece().substr(key.begin, key.len);
    if (key_str != name) {
      continue;
    }

    return std::string(url.query_piece().substr(value.begin, value.len));
  }
  return std::string();
}

bool SubscriptionRenderFrameObserver::IsValueAllowed(
    const std::string& purchase_token) const {
  if (purchase_token.length() > 0) {
    // Don't allow " in purchase token.
    // See https://github.com/brave/brave-browser/issues/27524
    std::size_t found = purchase_token.find("\"");
    if (found == std::string::npos) {
      return true;
    }
  }
  return false;
}

bool SubscriptionRenderFrameObserver::IsAllowed() {
  if (!skus::IsSafeOrigin(render_frame()->GetWebFrame()->GetSecurityOrigin())) {
    return false;
  }

  GURL current_url(
      render_frame()->GetWebFrame()->GetDocument().Url().GetString().Utf8());

  std::string intent = ExtractParam(current_url, kIntentParamName);
  std::string product = ExtractParam(current_url, kProductParamName);
  if (product == kProductVPNParamValue) {
    product_ = Product::kVPN;
  } else if (product == kProductLeoParamValue) {
    product_ = Product::kLeo;
  } else {
    product_ = std::nullopt;
  }
  return (intent == kIntentParamValue || intent == kIntentParamTestValue ||
          intent == kIntentParamValueLeo) &&
         product_.has_value();
}

void SubscriptionRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_subscription
