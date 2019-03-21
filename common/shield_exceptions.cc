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

#include "base/json/json_reader.h"
#include "base/values.h"

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

bool IsWhitelistedReferrer(const GURL& firstPartyOrigin,
    const GURL& subresourceUrl) {
  std::unique_ptr<base::Value> root = base::JSONReader::Read(
    "{"
    "    \"whitelist\": ["
    "        {"
    "            \"<all_urls>\": ["
    "                \"https://use.typekit.net/*\","
    "                \"https://api.geetest.com/*\","
    "                \"https://cloud.typography.com/*\""
    "            ]"
    "        },"
    "        {"
    "            \"https://www.facebook.com/\": ["
    "                \"https://*.fbcdn.net/*\""
    "            ]"
    "        },"
    "        {"
    "            \"https://accounts.google.com/\": ["
    "                \"https://content.googleapis.com/*\""
    "            ]"
    "        },"
    "        {"
    "            \"https://www.reddit.com/*\": ["
    "                \"https://www.redditmedia.com/*\","
    "                \"https://cdn.embedly.com/*\","
    "                \"https://imgur.com/*\""
    "            ]"
    "        }"
    "    ]"
    "}"
    );
  base::DictionaryValue* root_dict = nullptr;
  root->GetAsDictionary(&root_dict);
  base::ListValue* whitelist = nullptr;
  root_dict->GetList("whitelist", &whitelist);
  for (base::Value& origins : whitelist->GetList()) {
    base::DictionaryValue* origins_dict = nullptr;
    origins.GetAsDictionary(&origins_dict);
    for (const auto& it : origins_dict->DictItems()) {
      auto first_party_pattern = URLPattern(
        URLPattern::SCHEME_HTTP|URLPattern::SCHEME_HTTPS, it.first);
      if (first_party_pattern.MatchesURL(firstPartyOrigin)) {
        for (base::Value& subresource_value : it.second.GetList()) {
          auto subresource_pattern = URLPattern(
            URLPattern::SCHEME_HTTP|URLPattern::SCHEME_HTTPS,
            subresource_value.GetString());
          if (subresource_pattern.MatchesURL(subresourceUrl)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool IsWhitelistedCookieException(const GURL& firstPartyOrigin,
    const GURL& subresourceUrl, bool allow_google_auth) {
  // Note that there's already an exception for TLD+1, so don't add those here.
  // Check with the security team before adding exceptions.

  // 1st-party-INdependent whitelist
  std::vector<URLPattern> fpi_whitelist_patterns = {};
  if (allow_google_auth) {
    fpi_whitelist_patterns.push_back(URLPattern(URLPattern::SCHEME_ALL,
        "https://accounts.google.com/o/oauth2/*"));
  }
  bool any_match = std::any_of(fpi_whitelist_patterns.begin(),
      fpi_whitelist_patterns.end(),
      [&subresourceUrl](const URLPattern& pattern) {
        return pattern.MatchesURL(subresourceUrl);
      });
  if (any_match) {
    return true;
  }

  // 1st-party-dependent whitelist
  static std::map<GURL, std::vector<URLPattern> > whitelist_patterns = {};
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
