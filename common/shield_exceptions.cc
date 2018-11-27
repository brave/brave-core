/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/shield_exceptions.h"

#include <algorithm>
#include <map>
#include <vector>

#include "extensions/common/url_pattern.h"
#include "url/gurl.h"

namespace brave {

bool IsEmptyDataURLRedirect(const GURL& gurl) {
  static std::vector<std::string> hosts({
    "sp1.nypost.com",
    "sp.nasdaq.com"
  });
  return std::find(hosts.begin(), hosts.end(), gurl.host()) !=
      hosts.end();
}

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
    URLPattern(URLPattern::SCHEME_ALL, "https://www.lesechos.fr/xtcore.js"),
    URLPattern(URLPattern::SCHEME_ALL, "https://*.y8.com/js/sdkloader/outstream.js"),
    URLPattern(URLPattern::SCHEME_ALL, "https://pdfjs.robwu.nl/*")
  });
  return std::any_of(blocked_patterns.begin(), blocked_patterns.end(),
      [&gurl](URLPattern pattern){
        return pattern.MatchesURL(gurl);
      });
}

bool IsWhitelistedReferrer(const GURL& firstPartyOrigin,
    const GURL& subresourceUrl) {
  // Note that there's already an exception for TLD+1, so don't add those here.
  // Check with the security team before adding exceptions.

  // https://github.com/brave/browser-laptop/issues/5861
  // The below patterns are done to only allow the specific request
  // pattern, of reddit -> redditmedia -> embedly -> imgur.
  static auto redditPtrn = URLPattern(URLPattern::SCHEME_HTTPS, "https://www.reddit.com/*");
  static std::vector<URLPattern> reddit_embed_patterns({
    redditPtrn,
    URLPattern(URLPattern::SCHEME_HTTPS, "https://www.redditmedia.com/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://cdn.embedly.com/*"),
    URLPattern(URLPattern::SCHEME_HTTPS, "https://imgur.com/*")
  });

  if (redditPtrn.MatchesURL(firstPartyOrigin)) {
    bool is_reddit_embed = std::any_of(
      reddit_embed_patterns.begin(),
      reddit_embed_patterns.end(),
      [&subresourceUrl](URLPattern pattern){
        return pattern.MatchesURL(subresourceUrl);
      });
    if (is_reddit_embed) {
      return true;
    }
  }

  static std::map<GURL, std::vector<URLPattern> > whitelist_patterns_map = {{
      GURL("https://www.facebook.com/"), {
        URLPattern(URLPattern::SCHEME_HTTPS, "https://*.fbcdn.net/*"),
      }
    }
  };
  std::map<GURL, std::vector<URLPattern> >::iterator i =
      whitelist_patterns_map.find(firstPartyOrigin);
  if (i != whitelist_patterns_map.end()) {
    std::vector<URLPattern> &exceptions = i->second;
    bool any_match = std::any_of(exceptions.begin(), exceptions.end(),
        [&subresourceUrl](const URLPattern& pattern) {
          return pattern.MatchesURL(subresourceUrl);
        });
    if (any_match) {
      return true;
    }
  }

  // It's preferred to use specific_patterns below when possible
  static std::vector<URLPattern> whitelist_patterns({
    URLPattern(URLPattern::SCHEME_ALL, "https://use.typekit.net/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://api.geetest.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://cloud.typography.com/*")
  });
  return std::any_of(whitelist_patterns.begin(), whitelist_patterns.end(),
    [&subresourceUrl](URLPattern pattern){
      return pattern.MatchesURL(subresourceUrl);
    });
}

bool IsWhitelistedCookieExeption(const GURL& firstPartyOrigin,
    const GURL& subresourceUrl) {
  // Note that there's already an exception for TLD+1, so don't add those here.
  // Check with the security team before adding exceptions.
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

bool IsWidevineInstallableURL(const GURL& url) {
  static std::vector<URLPattern> patterns({
    URLPattern(URLPattern::SCHEME_ALL, "https://www.netflix.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://bitmovin.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://www.primevideo.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://www.spotify.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://shaka-player-demo.appspot.com/*"),
    URLPattern(URLPattern::SCHEME_ALL, "https://*.hulu.com/*"),
    // Used for tests
    URLPattern(URLPattern::SCHEME_ALL, "http://www.netflix.com:*/*")
  });
  return std::any_of(patterns.begin(), patterns.end(),
    [&url](URLPattern pattern){
      return pattern.MatchesURL(url);
    });
}

}
