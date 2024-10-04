/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_block_safebrowsing_urls.h"

#include <vector>

#include "base/no_destructor.h"
#include "extensions/common/url_pattern.h"
#include "net/base/net_errors.h"
#include "url/gurl.h"

namespace brave {

constexpr char kDummyUrl[] = "https://no-thanks.invalid";

bool IsSafeBrowsingReportingURL(const GURL& gurl) {
  static const base::NoDestructor<std::vector<URLPattern>> allowed_patterns({
      URLPattern(
          URLPattern::SCHEME_HTTPS,
          "https://sb-ssl.google.com/safebrowsing/clientreport/download*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
                 "https://safebrowsing.google.com/safebrowsing/clientreport/"
                 "crx-list-info*"),
  });
  static const base::NoDestructor<std::vector<URLPattern>> reporting_patterns({
      URLPattern(URLPattern::SCHEME_HTTPS,
                 "https://sb-ssl.google.com/safebrowsing/clientreport/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
                 "https://safebrowsing.google.com/safebrowsing/clientreport/*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
                 "https://safebrowsing.google.com/safebrowsing/report*"),
      URLPattern(URLPattern::SCHEME_HTTPS,
                 "https://safebrowsing.google.com/safebrowsing/uploads/*"),
  });

  if (std::any_of(
          allowed_patterns->begin(), allowed_patterns->end(),
          [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); })) {
    return false;
  }
  return std::any_of(
      reporting_patterns->begin(), reporting_patterns->end(),
      [&gurl](URLPattern pattern) { return pattern.MatchesURL(gurl); });
}

int OnBeforeURLRequest_BlockSafeBrowsingReportingURLs(const GURL& request_url,
                                                      GURL* new_url) {
  DCHECK(new_url);

  if (IsSafeBrowsingReportingURL(request_url)) {
    *new_url = GURL(kDummyUrl);
    return net::ERR_ABORTED;
  }

  return net::OK;
}

}  // namespace brave
