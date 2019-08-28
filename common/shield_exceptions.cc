/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"

#include <algorithm>
#include <map>
#include <vector>

#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace brave {

bool IsUAWhitelisted(const GURL& gurl) {
  static std::vector<URLPattern> whitelist_patterns({
    URLPattern(URLPattern::SCHEME_ALL, "https://*.adobe.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://*.duckduckgo.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://*.brave.com/*"),
    // For Widevine
    URLPattern(URLPattern::SCHEME_ALL, "https://*.netflix.com/*")
  });
  return std::any_of(whitelist_patterns.begin(), whitelist_patterns.end(),
      [&gurl](URLPattern pattern){
        return pattern.MatchesURL(gurl);
      });
}

bool IsBlockedResource(const GURL& gurl) {
  static std::vector<URLPattern> blocked_patterns({
      URLPattern(URLPattern::SCHEME_ALL, "https://pdfjs.robwu.nl/*")
  });
  return std::any_of(blocked_patterns.begin(), blocked_patterns.end(),
                     [&gurl](URLPattern pattern){
                       return pattern.MatchesURL(gurl);
                     });
}

bool IsWhitelistedFingerprintingException(const GURL& firstPartyOrigin,
    const GURL& subresourceUrl) {
  static std::map<GURL, std::vector<URLPattern> > whitelist_patterns = {
    {
      GURL("https://uphold.com/"),
      std::vector<URLPattern>({URLPattern(URLPattern::SCHEME_ALL,
            "https://uphold.netverify.com/*")})
    },
    {
      GURL("https://sandbox.uphold.com/"),
      std::vector<URLPattern>({URLPattern(URLPattern::SCHEME_ALL,
            "https://netverify.com/*")})
    }
  };
  std::map<GURL, std::vector<URLPattern> >::iterator i =
      whitelist_patterns.find(firstPartyOrigin);
  if (i == whitelist_patterns.end()) {
    return false;
  }
  std::vector<URLPattern> &exceptions = i->second;
  return std::any_of(exceptions.begin(), exceptions.end(),
      [&subresourceUrl](const URLPattern& pattern) {
        return pattern.MatchesURL(subresourceUrl);
      });
}

}  // namespace brave
