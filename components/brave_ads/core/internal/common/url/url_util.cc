/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util_internal.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_ads {

GURL GetUrlExcludingQuery(const GURL& url) {
  CHECK(url.is_valid());

  GURL::Replacements replacements;
  replacements.ClearQuery();
  return url.ReplaceComponents(replacements);
}

bool ShouldSupportUrl(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  if (url.HostIsIPAddress()) {
    return false;
  }

  if (url.has_port()) {
    return false;
  }

  if (url.has_username() || url.has_password()) {
    return false;
  }

  if (DoesETLDPlusOneContainWildcards(url)) {
    return false;
  }

  return url.SchemeIs(url::kHttpsScheme)
             ? HostHasRegistryControlledDomain(url.host_piece())
             : ShouldSupportInternalUrl(url);
}

bool MatchUrlPattern(const GURL& url, const std::string& pattern) {
  if (!url.is_valid() || pattern.empty()) {
    return false;
  }

  std::string escaped_pattern;
  base::ReplaceChars(pattern, "?", "\\?", &escaped_pattern);

  return base::MatchPattern(url.spec(), escaped_pattern);
}

bool MatchUrlPattern(const std::vector<GURL>& redirect_chain,
                     const std::string& pattern) {
  if (pattern.empty()) {
    return false;
  }

  const auto iter = base::ranges::find_if(
      redirect_chain, [&pattern](const GURL& redirect_chain_url) {
        return MatchUrlPattern(redirect_chain_url, pattern);
      });

  return iter != redirect_chain.cend();
}

bool SameDomainOrHost(const GURL& lhs, const GURL& rhs) {
  return net::registry_controlled_domains::SameDomainOrHost(
      lhs, rhs, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool DomainOrHostExists(const std::vector<GURL>& redirect_chain,
                        const GURL& url) {
  return base::ranges::any_of(
      redirect_chain, [&url](const GURL& redirect_chain_url) {
        return SameDomainOrHost(redirect_chain_url, url);
      });
}

}  // namespace brave_ads
