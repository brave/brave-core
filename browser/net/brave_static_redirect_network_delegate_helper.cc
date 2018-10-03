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
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  static URLPattern geo_pattern(URLPattern::SCHEME_HTTPS, kGeoLocationsPattern);
  static URLPattern safeBrowsing_pattern(URLPattern::SCHEME_HTTPS, kSafeBrowsingPrefix);

  if (geo_pattern.MatchesURL(request->url())) {
    *new_url = GURL(GOOGLEAPIS_ENDPOINT GOOGLEAPIS_API_KEY);
    return net::OK;
  }

  if (safeBrowsing_pattern.MatchesHost(request->url())) {
    replacements.SetHostStr(SAFEBROWSING_ENDPOINT);
    *new_url = request->url().ReplaceComponents(replacements);
    return net::OK;
  }

#if !defined(NDEBUG)
  GURL gurl = request->url();
  static std::vector<URLPattern> allowed_patterns({
    // Brave updates
    URLPattern(URLPattern::SCHEME_HTTPS, "https://go-updater.brave.com/*"),
    // Brave updates staging
    // In the future, we may want to exclude the value of the
    // BRAVE_REFERRALS_SERVER environment variable rather than
    // hardcoding the server name here
    URLPattern(URLPattern::SCHEME_HTTPS, "https://laptop-updates-staging.herokuapp.com/*"),
    // CRX file download
    URLPattern(URLPattern::SCHEME_HTTPS, "https://brave-core-ext.s3.brave.com/release/*"),
    // We do allow redirects to the Google update server for extensions we don't support
    URLPattern(URLPattern::SCHEME_HTTPS, "https://update.googleapis.com/service/update2"),

    // Ledger URLs
    URLPattern(URLPattern::SCHEME_HTTPS, "https://ledger.mercury.basicattentiontoken.org/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://balance.mercury.basicattentiontoken.org/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://publishers.basicattentiontoken.org/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://ledger-staging.mercury.basicattentiontoken.org/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://balance-staging.mercury.basicattentiontoken.org/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://publishers-staging.basicattentiontoken.org/*"),

    // Safe browsing
    URLPattern(URLPattern::SCHEME_HTTPS, "https://safebrowsing.brave.com/v4/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://ssl.gstatic.com/safebrowsing/*"),

    // ChromeCast network discovery
    URLPattern(URLPattern::SCHEME_HTTP, "http://*:8008/ssdp/device-desc.xml"),
    URLPattern(URLPattern::SCHEME_HTTP, "http://*:60000/upnp/dev/*/desc"),

    // Will be removed when https://github.com/brave/brave-browser/issues/663 is fixed
    URLPattern(URLPattern::SCHEME_HTTPS, "https://www.gstatic.com/*"),
  });
  // Check to make sure the URL being requested matches at least one of the allowed patterns.
  // Also allow invalid URLs (required for intranet_redirect_detector.cc to work)
  bool is_url_allowed = (!gurl.is_valid()) || std::any_of(allowed_patterns.begin(), allowed_patterns.end(),
    [&gurl](URLPattern pattern) {
      if (pattern.MatchesURL(gurl)) {
        return true;
      }
      return false;
    });
  DCHECK(is_url_allowed) << "URL not allowed from system network delegate: " << gurl;
#endif

  return net::OK;
}

}  // namespace brave
