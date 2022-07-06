/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/url/url_util.h"

#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ads {

GURL GetUrlWithEmptyQuery(const GURL& url) {
  return GURL(base::StrCat(
      {url.scheme(), url::kStandardSchemeSeparator, url.host(), url.path()}));
}

bool DoesUrlMatchPattern(const GURL& url, const std::string& pattern) {
  if (!url.is_valid() || pattern.empty()) {
    return false;
  }

  return base::MatchPattern(url.spec(), pattern);
}

bool SameDomainOrHost(const GURL& lhs, const GURL& rhs) {
  return net::registry_controlled_domains::SameDomainOrHost(
      lhs, rhs, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool DomainOrHostExists(const std::vector<GURL>& urls, const GURL& url) {
  for (const auto& element : urls) {
    if (SameDomainOrHost(element, url)) {
      return true;
    }
  }

  return false;
}

}  // namespace ads
