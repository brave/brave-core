/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/content/renderer/brave_render_frame_impl.h"

#include "brave/renderer/brave_content_settings_observer.h"
#include "brave/common/network_constants.h"
#include "brave/common/shield_exceptions.h"
#include "content/renderer/loader/web_url_request_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

using blink::WebString;
using namespace net::registry_controlled_domains;

namespace content {

  BraveRenderFrameImpl::BraveRenderFrameImpl(CreateParams params)
    : RenderFrameImpl(std::move(params)) {
  }

  void BraveRenderFrameImpl::ApplyReferrerBlocking(
      blink::WebURLRequest& request) {
    BraveContentSettingsObserver* observer =
        (BraveContentSettingsObserver*)ContentSettingsObserver::Get(this);
    std::string referrer =
        request.HttpHeaderField(WebString::FromUTF8(
            kRefererHeader)).Utf8();

    GURL site_for_cookies(request.SiteForCookies());
    GURL target_url(request.Url());
    if (referrer.length() > 0 &&
        // This does a TLD+1 comparison
        !SameDomainOrHost(target_url, GURL(referrer),
          INCLUDE_PRIVATE_REGISTRIES) &&
        !brave::IsWhitelistedCookieExeption(site_for_cookies, target_url) &&
        !observer->AllowReferrer(site_for_cookies)) {
      auto origin = blink::WebSecurityOrigin::Create(request.Url()).ToString();
      request.SetHTTPReferrer(origin, blink::kWebReferrerPolicyDefault);
    }
  }

  void BraveRenderFrameImpl::WillSendRequest(blink::WebURLRequest& request) {
    ApplyReferrerBlocking(request);
    RenderFrameImpl::WillSendRequest(request);
  }

}  // namespace content
