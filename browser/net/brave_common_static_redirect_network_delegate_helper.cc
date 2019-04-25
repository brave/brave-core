/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "brave/common/network_constants.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/url_pattern.h"

namespace brave {

// Update server checks happen from the profile context for admin policy
// installed extensions. Update server checks happen from the system context for
// normal update operations.
bool IsUpdaterURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns(
      {URLPattern(URLPattern::SCHEME_HTTPS,
                  std::string(component_updater::kUpdaterJSONDefaultUrl) + "*"),
       URLPattern(URLPattern::SCHEME_HTTP,
                  std::string(component_updater::kUpdaterJSONFallbackUrl) + "*"),
       URLPattern(
           URLPattern::SCHEME_HTTPS,
           std::string(extension_urls::kChromeWebstoreUpdateURL) + "*")});
  return std::any_of(
      updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

int OnBeforeURLRequest_CommonStaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  if (IsUpdaterURL(ctx->request_url)) {
    replacements.SetQueryStr(ctx->request_url.query_piece());
    ctx->new_url_spec = GURL(kBraveUpdatesExtensionsEndpoint)
                            .ReplaceComponents(replacements)
                            .spec();
    return net::OK;
  }
  return net::OK;
}

}  // namespace brave
