/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include "brave/common/network_constants.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"


namespace brave {

bool IsUpdaterURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(component_updater::kUpdaterDefaultUrl) + "*"),
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(component_updater::kUpdaterFallbackUrl) + "*"),
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(component_updater::kUpdaterDefaultUrlAlt) + "*"),
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(component_updater::kUpdaterFallbackUrlAlt) + "*"),
  });
  return std::any_of(updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern){
        return pattern.MatchesURL(gurl);
      });
}

int OnBeforeURLRequest_StaticRedirectWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  if (geo_pattern.MatchesURL(request->url())) {
    *new_url = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
    return net::OK;
  }
  if (IsUpdaterURL(request->url())) {
    GURL::Replacements replacements;
    replacements.SetQueryStr(request->url().query_piece());
    *new_url = GURL(kBraveUpdatesExtensionsEndpoint).ReplaceComponents(replacements);
    return net::OK;
  }
  return net::OK;
}

}  // namespace brave
