/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/renderer/brave_render_frame_impl.h"

#include "brave/renderer/brave_content_settings_observer.h"
#include "brave/common/origin_helper.h"
#include "brave/common/network_constants.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"

namespace content {

  BraveRenderFrameImpl::BraveRenderFrameImpl(CreateParams params)
    : RenderFrameImpl(std::move(params)) {
  }

  void BraveRenderFrameImpl::WillSendRequest(blink::WebURLRequest& request) {
    RenderFrameImpl::WillSendRequest(request);
    BraveContentSettingsObserver* observer =
        (BraveContentSettingsObserver*)ContentSettingsObserver::Get(this);
    std::string referrer =
        request.HttpHeaderField(blink::WebString::FromUTF8(
            kRefererHeader)).Utf8();
    bool same = false;
    if (referrer.length() > 0 &&
        brave::IsSameTLDPlus1(GURL(request.Url()), GURL(referrer), &same) &&
        !same && !observer->AllowReferrer()) {
      auto origin = blink::WebSecurityOrigin::Create(request.Url()).ToString();
      request.SetHTTPReferrer(origin, blink::kWebReferrerPolicyDefault);
    }
  }

}  // namespace content
