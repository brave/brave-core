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
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/url_pattern.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/extension_urls.h"
#endif

namespace brave {

// Update server checks happen from the profile context for admin policy
// installed extensions. Update server checks happen from the system context for
// normal update operations.
bool IsUpdaterURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns(
      {URLPattern(URLPattern::SCHEME_HTTPS,
                  std::string(component_updater::kUpdaterJSONDefaultUrl) + "*"),
       URLPattern(
           URLPattern::SCHEME_HTTP,
           std::string(component_updater::kUpdaterJSONFallbackUrl) + "*"),
#if BUILDFLAG(ENABLE_EXTENSIONS)
       URLPattern(
           URLPattern::SCHEME_HTTPS,
           std::string(extension_urls::kChromeWebstoreUpdateURL) + "*")
#endif
  });
  return std::any_of(
      updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

int OnBeforeURLRequest_CommonStaticRedirectWork(
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx) {
  GURL::Replacements replacements;
  static URLPattern googleapis_pattern(
    URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kGoogleAPIPrefix);
  static URLPattern chromecast_pattern(
    URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kChromeCastPrefix);

  if (IsUpdaterURL(ctx->request_url)) {
    replacements.SetQueryStr(ctx->request_url.query_piece());
    ctx->new_url_spec = GURL(kBraveUpdatesExtensionsEndpoint)
                            .ReplaceComponents(replacements)
                            .spec();
    return net::OK;
  }

  if (googleapis_pattern.MatchesHost(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveGoogleAPIProxy);
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  if (chromecast_pattern.MatchesURL(ctx->request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    ctx->new_url_spec = ctx->request_url.ReplaceComponents(replacements).spec();
    return net::OK;
  }

  return net::OK;
}

}  // namespace brave
