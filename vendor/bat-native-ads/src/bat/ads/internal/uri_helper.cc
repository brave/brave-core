/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "bat/ads/internal/uri_helper.h"
#include "url/url_constants.h"
#include "url/gurl.h"
#include "third_party/re2/src/re2/re2.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace helper {

std::string Uri::GetUri(
    const std::string& url) {
  auto http_scheme = std::string(url::kHttpScheme) +
      std::string(url::kStandardSchemeSeparator);

  auto https_scheme = std::string(url::kHttpsScheme) +
      std::string(url::kStandardSchemeSeparator);

  if (url.find(http_scheme) != std::string::npos &&
      url.find(https_scheme) != std::string::npos) {
    return https_scheme + url;
  }

  return url;
}

bool Uri::MatchesWildcard(
    const std::string& url,
    const std::string& pattern) {
  DCHECK(!url.empty());
  DCHECK(!pattern.empty());

  if (url.empty() || pattern.empty()) {
    return false;
  }

  std::string lowercase_url = url;
  std::transform(lowercase_url.begin(), lowercase_url.end(),
      lowercase_url.begin(), ::tolower);

  std::string lowercase_pattern = pattern;
  std::transform(lowercase_pattern.begin(), lowercase_pattern.end(),
      lowercase_pattern.begin(), ::tolower);

  auto quoted_lowercase_pattern = RE2::QuoteMeta(lowercase_pattern);
  RE2::GlobalReplace(&quoted_lowercase_pattern, "\\\\\\*", ".*");

  return RE2::FullMatch(lowercase_url, quoted_lowercase_pattern);
}

bool Uri::MatchesDomainOrHost(
      const std::string& url1,
      const std::string& url2) {
  return SameDomainOrHost(GURL(url1), GURL(url2),
      net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace helper
