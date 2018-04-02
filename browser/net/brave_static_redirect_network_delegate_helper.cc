/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"


namespace brave {

int OnBeforeURLRequest_StaticRedirectWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveURLRequestContext> ctx) {
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  if (geo_pattern.MatchesURL(request->url())) {
    *new_url = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
    return net::OK;
  }
  return net::OK;
}

}  // namespace brave
