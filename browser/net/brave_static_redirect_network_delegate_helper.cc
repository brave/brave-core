/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"

namespace brave {

int OnBeforeURLRequest_StaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  static URLPattern safeBrowsing_pattern(URLPattern::SCHEME_HTTPS, kSafeBrowsingPrefix);

  if (geo_pattern.MatchesURL(ctx->request_url)) {
    ctx->new_url_spec = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY).spec();
    return net::OK;
  }

  if (safeBrowsing_pattern.MatchesHost(ctx->request_url)) {
    replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }
  return net::OK;
}

}  // namespace brave
