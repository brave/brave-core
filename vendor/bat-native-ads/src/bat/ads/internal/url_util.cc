/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/url_util.h"

#include "base/check.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ads {

bool DoesUrlMatchPattern(const std::string& url, const std::string& pattern) {
  if (url.empty() || pattern.empty()) {
    return false;
  }

  std::string quoted_pattern = RE2::QuoteMeta(pattern);
  RE2::GlobalReplace(&quoted_pattern, "\\\\\\*", ".*");

  return RE2::FullMatch(url, quoted_pattern);
}

bool DoesUrlHaveSchemeHTTPOrHTTPS(const std::string& url) {
  DCHECK(!url.empty());

  return GURL(url).SchemeIsHTTPOrHTTPS();
}

std::string GetHostFromUrl(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    return "";
  }

  return gurl.host();
}

bool SameDomainOrHost(const std::string& lhs, const std::string& rhs) {
  return net::registry_controlled_domains::SameDomainOrHost(
      GURL(lhs), GURL(rhs),
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool DomainOrHostExists(const std::vector<std::string>& urls,
                        const std::string& url) {
  for (const auto& element : urls) {
    if (SameDomainOrHost(element, url)) {
      return true;
    }
  }

  return false;
}

}  // namespace ads
