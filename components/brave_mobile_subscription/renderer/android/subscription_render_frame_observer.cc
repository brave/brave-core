// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_mobile_subscription/renderer/android/subscription_render_frame_observer.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/skus/renderer/skus_utils.h"
#include "brave/gin/converter_specializations.h"
#include "build/build_config.h"
#include "content/public/renderer/render_frame.h"
#include "gin/function_template.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_util.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
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
inline constexpr char kResultLandingPagePathLeo[] = "/order-link/";

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

  if (ai_chat::features::IsAIChatEnabled() && product_ == Product::kLeo) {
    if (!ai_chat_subscription_.is_bound()) {
      render_frame()->GetBrowserInterfaceBroker().GetInterface(
          ai_chat_subscription_.BindNewPipeAndPassReceiver());
    }
    bound |= ai_chat_subscription_.is_bound();
  }
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
    if (ai_chat_subscription_.is_bound()) {
      // Inject only linkResult object on the
      // https://account.brave.com/order-link/?product=leo page
      // and get purchase token id only on
      // https://account.brave.com?intent=link-order&product=leo page
      if (page_ == Page::kResultLandingPage) {
        AddJavaScriptObjectToFrame(context);
      } else if (page_ == Page::kInitialLandingPage) {
        ai_chat_subscription_->GetPurchaseTokenOrderId(base::BindOnce(
            &SubscriptionRenderFrameObserver::OnGetPurchaseTokenOrderId,
            weak_factory_.GetWeakPtr()));
      }
    }
  }
}

void SubscriptionRenderFrameObserver::AddJavaScriptObjectToFrame(
    v8::Local<v8::Context> context) {
  if (!render_frame() || context.IsEmpty()) {
    return;
  }

  v8::Isolate* isolate =
      render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Context::Scope context_scope(context);

  CreateLinkResultObject(isolate, context);
}

void SubscriptionRenderFrameObserver::CreateLinkResultObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Object> link_result_obj;
  v8::Local<v8::Value> link_result_value;
  if (!global->Get(context, gin::StringToV8(isolate, "linkResult"))
           .ToLocal(&link_result_value) ||
      !link_result_value->IsObject()) {
    link_result_obj = v8::Object::New(isolate);
    global
        ->Set(context, gin::StringToSymbol(isolate, "linkResult"),
              link_result_obj)
        .Check();
    BindFunctionToObject(
        isolate, link_result_obj, "setStatus",
        base::BindRepeating(&SubscriptionRenderFrameObserver::SetLinkStatus,
                            base::Unretained(this)));
  }
}

template <typename Sig>
void SubscriptionRenderFrameObserver::BindFunctionToObject(
    v8::Isolate* isolate,
    v8::Local<v8::Object> javascript_object,
    const std::string& name,
    const base::RepeatingCallback<Sig>& callback) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  // Get the isolate associated with this object.
  javascript_object
      ->Set(context, gin::StringToSymbol(isolate, name),
            gin::CreateFunctionTemplate(isolate, callback)
                ->GetFunction(context)
                .ToLocalChecked())
      .Check();
}

void SubscriptionRenderFrameObserver::SetLinkStatus(
    base::Value::Dict status_dict) {
  // The payload looks like that
  // { status: <value>}
  // where value 0 means it's not linked otherwise it's linked.
  // VPN uses a different way to detect that.
  // It uses Guardian backend call for that.
  if (product_ != Product::kLeo || !ai_chat_subscription_.is_bound() ||
      status_dict.empty()) {
    return;
  }

  ai_chat_subscription_->SetLinkStatus(
      status_dict.FindInt("status").value_or(0));
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
  url::Component query(0, static_cast<int>(url.query().length())), key, value;
  while (url::ExtractQueryKeyValue(url.query(), &query, &key, &value)) {
    std::string_view key_str = url.query().substr(key.begin, key.len);
    if (key_str != name) {
      continue;
    }

    return std::string(url.query().substr(value.begin, value.len));
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
    // We allow to inject linkResult object if intent value is empty and path is
    // /order-link/ as https://account.brave.com?intent=link-order&product=leo
    // gets redirected to https://account.brave.com/order-link/?product=leo for
    // an actual linking where we should receive the result of linking
    if (intent.empty()) {
      std::string_view path = current_url.has_path() ? current_url.path() : "";
      if (path == kResultLandingPagePathLeo) {
        page_ = Page::kResultLandingPage;
      }
    } else {
      page_ = Page::kInitialLandingPage;
    }
  } else {
    product_ = std::nullopt;
  }
  return (intent == kIntentParamValue || intent == kIntentParamTestValue ||
          intent == kIntentParamValueLeo ||
          (intent.empty() && page_.has_value() &&
           page_ == Page::kResultLandingPage)) &&
         product_.has_value();
}

void SubscriptionRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_subscription
