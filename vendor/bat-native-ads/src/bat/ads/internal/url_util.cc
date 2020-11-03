/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/url_util.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/url_constants.h"
#include "bat/ads/internal/logging.h"

namespace ads {

bool UrlMatchesPattern(
    const std::string& url,
    const std::string& pattern) {
  if (url.empty() || pattern.empty()) {
    return false;
  }

  std::string quoted_pattern = RE2::QuoteMeta(pattern);
  RE2::GlobalReplace(&quoted_pattern, "\\\\\\*", ".*");

  return RE2::FullMatch(url, quoted_pattern);
}

bool UrlHasScheme(
    const std::string& url) {
  DCHECK(!url.empty());

  return GURL(url).SchemeIsHTTPOrHTTPS();
}

std::string GetUrlHost(
    const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    return "";
  }

  return gurl.host();
}

bool SameSite(
    const std::string& url1,
    const std::string& url2) {
  return net::registry_controlled_domains::SameDomainOrHost(GURL(url1),
      GURL(url2), net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

std::map<std::string, std::string> HeadersToMap(
    const std::vector<std::string>& headers) {
  std::map<std::string, std::string> normalized_headers;

  for (const auto& header : headers) {
    const std::vector<std::string> components = base::SplitString(header,
        ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    if (components.size() != 2) {
      NOTREACHED();
      continue;
    }

    const std::string key = components.at(0);
    const std::string value = components.at(1);

    normalized_headers[key] = value;
  }

  return normalized_headers;
}

}  // namespace ads
