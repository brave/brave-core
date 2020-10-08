/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/common/ipfs_utils.h"

#include <vector>

#include "brave/components/ipfs/common/ipfs_constants.h"
#include "brave/components/ipfs/common/translate_ipfs_uri.h"
#include "extensions/common/url_pattern.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace ipfs {

// static
bool IpfsUtils::IsIPFSURL(const GURL& gurl) {
  static std::vector<URLPattern> updater_patterns(
      {URLPattern(URLPattern::SCHEME_ALL, "*://*/ipfs/*"),
       URLPattern(URLPattern::SCHEME_ALL, "*://*/ipns/*")});
  return std::any_of(
      updater_patterns.begin(), updater_patterns.end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

// static
bool IpfsUtils::IsDefaultGatewayURL(const GURL& url) {
  return url.GetOrigin() == GURL(kDefaultIPFSGateway);
}

// static
bool IpfsUtils::IsLocalGatewayURL(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS() && net::IsLocalhost(url) &&
         url.port_piece() == GURL(kDefaultIPFSLocalGateway).port_piece();
}

// static
bool IpfsUtils::IsIPFSScheme(const GURL& url) {
  return url.SchemeIs(kIPFSScheme) || url.SchemeIs(kIPNSScheme);
}

// static
GURL IpfsUtils::ToPublicGatewayURL(const GURL& url) {
  DCHECK(IsIPFSScheme(url) || IsIPFSURL(url));
  GURL new_url;

  // For ipfs/ipns schemes, use TranslateIPFSURI directly.
  if (IsIPFSScheme(url) && TranslateIPFSURI(url, &new_url, false)) {
    return new_url;
  }

  // For local gateway addresses, replace its scheme, host, port with the
  // public gateway URL.
  if (IsLocalGatewayURL(url)) {
    GURL::Replacements replacements;
    GURL gateway_url = GURL(kDefaultIPFSGateway);
    replacements.ClearPort();
    replacements.SetSchemeStr(gateway_url.scheme_piece());
    replacements.SetHostStr(gateway_url.host_piece());
    return url.ReplaceComponents(replacements);
  }

  return new_url;
}

}  // namespace ipfs
