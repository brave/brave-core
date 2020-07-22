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
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/common/network_constants.h"
#include "brave/components/brave_component_updater/browser/features.h"
#include "brave/components/brave_component_updater/browser/switches.h"
#include "components/component_updater/component_updater_url_constants.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/extension_urls.h"
#endif

namespace brave {

const char kUpdaterTestingEndpoint[] = "test.updater.com";

namespace {

bool g_updater_url_host_for_testing_ = false;

std::string GetUpdateURLHost() {
  if (g_updater_url_host_for_testing_)
    return kUpdaterTestingEndpoint;

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(brave_component_updater::kUseGoUpdateDev) &&
      !base::FeatureList::IsEnabled(
          brave_component_updater::kUseDevUpdaterUrl)) {
    return UPDATER_PROD_ENDPOINT;
  }
  return UPDATER_DEV_ENDPOINT;
}

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

bool RewriteBugReportingURL(const GURL& request_url, GURL* new_url) {
  GURL url("https://github.com/brave/brave-browser/issues/new");
  std::string query = "title=Crash%20Report&labels=crash";
  // We are expecting 3 query keys: comment, template, and labels
  base::StringPairs pairs;
  if (!base::SplitStringIntoKeyValuePairs(request_url.query(), '=', '&',
                                          &pairs) || pairs.size() != 3) {
      return false;
  }
  for (const auto& pair : pairs) {
    if (pair.first == "comment") {
      query += "&body=" + pair.second;
      base::ReplaceSubstringsAfterOffset(&query, 0, "Chrome", "Brave");
    } else if (pair.first != "template" && pair.first != "labels") {
      return false;
    }
  }

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  *new_url = url.ReplaceComponents(replacements);
  return true;
}

}  // namespace

void SetUpdateURLHostForTesting(bool testing) {
  g_updater_url_host_for_testing_ = testing;
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
  static URLPattern bugsChromium_pattern(
      URLPattern::SCHEME_HTTP | URLPattern::SCHEME_HTTPS,
      "*://bugs.chromium.org/p/chromium/issues/entry?*");

  if (IsUpdaterURL(request_url)) {
    auto update_host = GetUpdateURLHost();
    if (!update_host.empty()) {
      replacements.SetQueryStr(request_url.query_piece());
      *new_url = GURL(update_host).ReplaceComponents(replacements);
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

  if (bugsChromium_pattern.MatchesURL(request_url)) {
    if (RewriteBugReportingURL(request_url, new_url))
      return net::OK;
  }

  return net::OK;
}


}  // namespace brave
