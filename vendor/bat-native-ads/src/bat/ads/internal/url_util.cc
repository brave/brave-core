/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/url_util.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ads {

std::string GetUrlWithScheme(
    const std::string& url) {
  if (!base::StartsWith(url, url::kHttpScheme,
          base::CompareCase::INSENSITIVE_ASCII) &&
      !base::StartsWith(url, url::kHttpsScheme,
          base::CompareCase::INSENSITIVE_ASCII)) {
    return base::StringPrintf("%s%s%s", url::kHttpsScheme,
        url::kStandardSchemeSeparator, url.c_str());
  }

  return url;
}

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

bool SameSite(
      const std::string& url1,
      const std::string& url2) {
  return net::registry_controlled_domains::SameDomainOrHost(GURL(url1),
      GURL(url2), net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

}  // namespace ads
