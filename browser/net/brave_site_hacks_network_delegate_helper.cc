/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_site_hacks_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/constants/url_constants.h"
#include "brave/components/query_filter/utils.h"
#include "content/public/common/referrer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/common/loader/network_utils.h"
#include "third_party/blink/public/common/loader/referrer_utils.h"

namespace brave {

namespace {

bool ApplyPotentialReferrerBlock(std::shared_ptr<BraveRequestInfo> ctx) {
  if (ctx->tab_origin.SchemeIs(kChromeExtensionScheme)) {
    return false;
  }

  if (ctx->resource_type == blink::mojom::ResourceType::kMainFrame ||
      ctx->resource_type == blink::mojom::ResourceType::kSubFrame) {
    // Frame navigations are handled in content::NavigationRequest.
    return false;
  }

  content::Referrer new_referrer;
  if (brave_shields::MaybeChangeReferrer(
          ctx->allow_referrers, ctx->allow_brave_shields, GURL(ctx->referrer),
          ctx->request_url, &new_referrer)) {
    ctx->new_referrer = new_referrer.url;
    return true;
  }
  return false;
}

}  // namespace

int OnBeforeURLRequest_SiteHacksWork(const ResponseCallback& next_callback,
                                     std::shared_ptr<BraveRequestInfo> ctx) {
  ApplyPotentialReferrerBlock(ctx);
  if (ctx->allow_brave_shields) {
    auto filtered_url = query_filter::MaybeApplyQueryStringFilter(
        ctx->initiator_url, ctx->redirect_source, ctx->request_url, ctx->method,
        ctx->internal_redirect);

    if (filtered_url.has_value()) {
      ctx->new_url_spec = filtered_url.value().spec();
    }
  }
  return net::OK;
}

int OnBeforeStartTransaction_SiteHacksWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  // Special case for handling top-level redirects. There is no other way to
  // normally change referrer in net::URLRequest during redirects
  // (except using network::mojom::TrustedURLLoaderHeaderClient, which
  // will affect performance).
  // Note that this code only affects "Referer" header sent via network - we
  // handle document.referer in content::NavigationRequest (see also
  // |BraveContentBrowserClient::MaybeHideReferrer|).
  if (!ctx->allow_referrers && ctx->allow_brave_shields &&
      ctx->redirect_source.is_valid() &&
      ctx->resource_type == blink::mojom::ResourceType::kMainFrame &&
      !brave_shields::IsSameOriginNavigation(ctx->redirect_source,
                                             ctx->request_url)) {
    // This is a hack that notifies the network layer.
    ctx->removed_headers.insert("X-Brave-Cap-Referrer");
  }
  return net::OK;
}

}  // namespace brave
