/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/uri_helper.h"

#include "url/url_constants.h"
#include "url/gurl.h"

#include "third_party/re2/src/re2/re2.h"

namespace helper {

std::string Uri::GetUri(const std::string& url) {
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

// TODO(tmancey): Refactor to use base::
bool Uri::MatchWildcard(
    const std::string& url,
    const std::string& pattern) {
  std::string url_scheme = GURL(url).scheme();
  std::transform(url_scheme.begin(), url_scheme.end(),
      url_scheme.begin(), ::tolower);

  std::string url_host = GURL(url).host();
  std::transform(url_host.begin(), url_host.end(),
      url_host.begin(), ::tolower);

  std::string pattern_scheme = GURL(pattern).scheme();
  std::transform(pattern_scheme.begin(), pattern_scheme.end(),
      pattern_scheme.begin(), ::tolower);

  std::string pattern_host = GURL(pattern).host();
  std::transform(pattern_host.begin(), pattern_host.end(),
      pattern_host.begin(), ::tolower);

  if (url_scheme != "http" && url_scheme != "https") {
    return false;
  }

  if (pattern_scheme != "http" && pattern_scheme != "https") {
    return false;
  }

  if (url_host != pattern_host) {
    return false;
  }

  auto wildcard_pattern = RE2::QuoteMeta(pattern);
  RE2::GlobalReplace(&wildcard_pattern, "\\\\\\*", ".*");

  return RE2::FullMatch(url, wildcard_pattern);
}

}  // namespace helper
