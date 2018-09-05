/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include "brave/common/network_constants.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/url_pattern.h"
#include "net/url_request/url_request.h"


namespace brave {

// Update server checks happen from the profile context for admin policy installed extensions.
// Update server checks happen from the system context for normal update operations.
bool IsUpdaterURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(component_updater::kUpdaterDefaultUrl) + "*"),
      URLPattern(URLPattern::SCHEME_HTTP, std::string(component_updater::kUpdaterFallbackUrl) + "*"),
      URLPattern(URLPattern::SCHEME_HTTPS, std::string(extension_urls::kChromeWebstoreUpdateURL) + "*")
  });
  bool braveRedirect = gurl.query().find("braveRedirect=true") != std::string::npos;
  return std::any_of(updater_patterns.begin(), updater_patterns.end(),
      [&gurl, braveRedirect](URLPattern pattern) {
        return !braveRedirect && pattern.MatchesURL(gurl);
      });
}

int OnBeforeURLRequest_CommonStaticRedirectWork(
    net::URLRequest* request,
    GURL* new_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  if (IsUpdaterURL(request->url())) {
    replacements.SetQueryStr(request->url().query_piece());
    *new_url = GURL(kBraveUpdatesExtensionsEndpoint).ReplaceComponents(replacements);
    return net::OK;
  }
  return net::OK;
}

}  // namespace brave
