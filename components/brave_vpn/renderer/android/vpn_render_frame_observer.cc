// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/renderer/android/vpn_render_frame_observer.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/renderer/skus_utils.h"
#include "build/build_config.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_util.h"

namespace brave_vpn {

namespace {

char kIntentParamName[] = "intent";
char kIntentParamValue[] = "connect-receipt";
char kIntentParamTestValue[] = "connect-receipt-test";
char kProductParamName[] = "product";
char kProductParamValue[] = "vpn";

}  // namespace

VpnRenderFrameObserver::VpnRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

VpnRenderFrameObserver::~VpnRenderFrameObserver() = default;

bool VpnRenderFrameObserver::EnsureConnected() {
  if (!vpn_service_.is_bound()) {
    render_frame()->GetBrowserInterfaceBroker()->GetInterface(
        vpn_service_.BindNewPipeAndPassReceiver());
  }
  return vpn_service_.is_bound();
}

void VpnRenderFrameObserver::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (!render_frame()->IsMainFrame() || world_id_ != world_id) {
    return;
  }

  if (!IsAllowed()) {
    return;
  }

  auto connected = EnsureConnected();
  if (!connected) {
    LOG(ERROR) << "Failed to establish connection to BraveVpnService";
    return;
  }

#if BUILDFLAG(IS_ANDROID)
  vpn_service_->GetPurchaseToken(base::BindOnce(
      &VpnRenderFrameObserver::OnGetPurchaseToken, weak_factory_.GetWeakPtr()));
#endif
}

void VpnRenderFrameObserver::OnGetPurchaseToken(
    const std::string& purchase_token) {
  if (!IsAllowed()) {
    return;
  }
  auto* frame = render_frame();
  if (frame) {
    if (IsValueAllowed(purchase_token)) {
      std::u16string set_local_storage =
          base::StrCat({u"window.localStorage.setItem(\"braveVpn.receipt\", \"",
                        base::UTF8ToUTF16(purchase_token), u"\");"});
      frame->ExecuteJavaScript(set_local_storage);
    }
  }
}

std::string VpnRenderFrameObserver::ExtractParam(
    const GURL& url,
    const std::string& name) const {
  url::Component query(0, static_cast<int>(url.query_piece().length())), key,
      value;
  while (url::ExtractQueryKeyValue(url.query_piece().data(), &query, &key,
                                   &value)) {
    std::string_view key_str = url.query_piece().substr(key.begin, key.len);
    if (key_str != name) {
      continue;
    }

    return std::string(url.query_piece().substr(value.begin, value.len));
  }
  return std::string();
}

bool VpnRenderFrameObserver::IsValueAllowed(
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

bool VpnRenderFrameObserver::IsAllowed() {
  DCHECK(brave_vpn::IsBraveVPNFeatureEnabled());

  if (!skus::IsSafeOrigin(render_frame()->GetWebFrame()->GetSecurityOrigin())) {
    return false;
  }

  GURL current_url(
      render_frame()->GetWebFrame()->GetDocument().Url().GetString().Utf8());

  std::string intent = ExtractParam(current_url, kIntentParamName);
  std::string product = ExtractParam(current_url, kProductParamName);
  return (intent == kIntentParamValue || intent == kIntentParamTestValue) &&
         product == kProductParamValue;
}

void VpnRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_vpn
