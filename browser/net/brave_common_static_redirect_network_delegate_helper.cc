/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_common_static_redirect_network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/constants/network_constants.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"

namespace brave {

namespace {

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
