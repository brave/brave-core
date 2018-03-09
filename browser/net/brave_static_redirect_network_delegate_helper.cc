/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include <string>
#include <vector>

#include "brave/common/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"


namespace brave {

bool IsEmptyDataURLRedirect(const GURL& gurl) {
  static std::vector<std::string> hosts({
    "sp1.nypost.com",
    "sp.nasdaq.com"
  });
  return std::find(hosts.begin(), hosts.end(), gurl.host()) !=
      hosts.end();
}

int OnBeforeURLRequest_StaticRedirectWork(
  bool is_system_check,
  net::URLRequest* request,
  GURL* new_url,
  const ResponseCallback& next_callback,
  std::shared_ptr<OnBeforeURLRequestContext> ctx) {
  if (is_system_check) {
    static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS,
        "https://www.googleapis.com/geolocation/v1/geolocate?key=*");
    if (geo_pattern.MatchesURL(request->url())) {
      *new_url = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
      return net::OK;
    }
  }

  if (IsEmptyDataURLRedirect(request->url())) {
    *new_url = GURL(kEmptyDataURI);
    return net::OK;
  }

  return net::OK;
}

}  // namespace brave
