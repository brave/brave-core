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
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace brave_vpn {

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
      &VpnRenderFrameObserver::OnGetPurchaseToken, base::Unretained(this)));
}

void VpnRenderFrameObserver::OnGetPurchaseToken(
    const std::string& purchase_token) {
  auto* frame = render_frame();
  if (frame && purchase_token.length() > 0) {
    std::u16string set_local_storage(
        u"window.localStorage.setItem(\"braveVpn.receipt\", \"");
    set_local_storage.append(base::UTF8ToUTF16(purchase_token));
    set_local_storage.append(u"\");");
    frame->ExecuteJavaScript(set_local_storage);
  }
}

bool VpnRenderFrameObserver::IsAllowed() {
  DCHECK(brave_vpn::IsBraveVPNEnabled());
  // NOTE: please open a security review when appending to this list.
  // TODO(bsclifton): this list also used in:
  // components\skus\renderer\skus_render_frame_observer.cc
  // maybe it can be pulled to a central place
  static base::NoDestructor<std::vector<blink::WebSecurityOrigin>> safe_origins{
      {{blink::WebSecurityOrigin::Create(GURL("https://account.brave.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.bravesoftware.com"))},
       {blink::WebSecurityOrigin::Create(
           GURL("https://account.brave.software"))}}};

  bool allowed = false;
  const blink::WebSecurityOrigin& visited_origin =
      render_frame()->GetWebFrame()->GetSecurityOrigin();
  for (const blink::WebSecurityOrigin& safe_origin : *safe_origins) {
    if (safe_origin.IsSameOriginWith(visited_origin)) {
      allowed = true;
    }
  }

  // TODO: also check query string param (how??)
  // need to make sure it has
  // "?intent=connect-receipt&product=vpn"
  // if (allowed && !has query string) {
  //    return false;
  // }
  //
  return allowed;
}

void VpnRenderFrameObserver::OnDestruct() {
  delete this;
}

}  // namespace brave_vpn
