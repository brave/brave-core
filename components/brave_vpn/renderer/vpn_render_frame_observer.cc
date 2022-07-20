// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_vpn/renderer/vpn_render_frame_observer.h"

#include <string>
#include <vector>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_vpn/brave_vpn_utils.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/skus/common/skus_utils.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/url_util.h"

namespace brave_vpn {

namespace {

char kIntentParamName[] = "intent";
char kIntentParamValue[] = "connect-receipt";
char kProductParamName[] = "product";
char kProductParamValue[] = "vpn";

bool ExtractQueryParamValue(base::StringPiece str,
                            const std::string& name,
                            std::string* result) {
  url::Component query(0, static_cast<int>(str.length())), key, value;
  while (url::ExtractQueryKeyValue(str.data(), &query, &key, &value)) {
    base::StringPiece key_str = str.substr(key.begin, key.len);
    if (key_str != name)
      continue;

    *result = std::string(str.substr(value.begin, value.len));
    return true;
  }
  return false;
}

}  // namespace

VpnRenderFrameObserver::VpnRenderFrameObserver(
    content::RenderFrame* render_frame,
    int32_t world_id)
    : RenderFrameObserver(render_frame), world_id_(world_id) {}

VpnRenderFrameObserver::~VpnRenderFrameObserver() {}

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
  if (!render_frame()->IsMainFrame() || world_id_ != world_id)
    return;

  if (!IsAllowed())
    return;

  auto connected = EnsureConnected();
  if (!connected) {
    LOG(ERROR) << "Failed to establish connection to BraveVpnService";
    return;
  }

  vpn_service_->GetPurchaseToken(base::BindOnce(
      &VpnRenderFrameObserver::OnGetPurchaseToken, weak_factory_.GetWeakPtr()));
}

void VpnRenderFrameObserver::OnGetPurchaseToken(
    const std::string& purchase_token) {
  if (!IsAllowed())
    return;
  auto* frame = render_frame();
  if (frame && purchase_token.length() > 0) {
    std::u16string set_local_storage(
        u"window.sessionStorage.setItem(\"braveVpn.receipt\", \"");
    set_local_storage.append(base::UTF8ToUTF16(purchase_token));
    set_local_storage.append(u"\");");
    frame->ExecuteJavaScript(set_local_storage);
  }
}

bool VpnRenderFrameObserver::IsAllowed() {
  DCHECK(brave_vpn::IsBraveVPNEnabled());

  if (!skus::IsSafeOrigin(render_frame()->GetWebFrame()->GetSecurityOrigin()))
    return false;

  GURL current_url(
      render_frame()->GetWebFrame()->GetDocument().Url().GetString().Utf8());

  std::string intent;
  if (!ExtractQueryParamValue(current_url.query_piece(), kIntentParamName,
                              &intent) ||
      intent != kIntentParamValue) {
    return false;
  }
  std::string product;
  if (!ExtractQueryParamValue(current_url.query_piece(), kProductParamName,
                              &product) ||
      product != kProductParamValue) {
    return false;
  }
  return true;
}

void VpnRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_vpn
