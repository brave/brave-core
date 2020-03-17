/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/common/brave_features.h"
#include "brave/common/brave_switches.h"
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
  GURL new_url;
  int rc = OnBeforeURLRequest_CommonStaticRedirectWorkForGURL(ctx->request_url,
                                                              &new_url);
  if (!new_url.is_empty()) {
    ctx->new_url_spec = new_url.spec();
  }
  return rc;
}

int OnBeforeURLRequest_CommonStaticRedirectWorkForGURL(
    const GURL& request_url,
    GURL* new_url) {
  DCHECK(new_url);

  GURL::Replacements replacements;
  static URLPattern chromecast_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kChromeCastPrefix);
  static URLPattern clients4_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS, kClients4Prefix);

  if (IsUpdaterURL(request_url)) {
    replacements.SetQueryStr(request_url.query_piece());
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    if (!command_line.HasSwitch(switches::kUseGoUpdateDev) &&
        !base::FeatureList::IsEnabled(features::kUseDevUpdaterUrl)) {
      *new_url = GURL(kBraveUpdatesExtensionsProdEndpoint)
                            .ReplaceComponents(replacements);
    } else {
      *new_url = GURL(kBraveUpdatesExtensionsDevEndpoint)
                            .ReplaceComponents(replacements);
    }
    return net::OK;
  }

  if (chromecast_pattern.MatchesURL(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveRedirectorProxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  if (clients4_pattern.MatchesHost(request_url)) {
    replacements.SetSchemeStr("https");
    replacements.SetHostStr(kBraveClients4Proxy);
    *new_url = request_url.ReplaceComponents(replacements);
    return net::OK;
  }

  return net::OK;
}


}  // namespace brave
