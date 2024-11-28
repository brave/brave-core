/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util_internal.h"

#include <string>

#include "base/containers/contains.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kChromeScheme[] = "chrome";

constexpr char kRewardsHostName[] = "rewards";
constexpr char kSyncHostName[] = "sync";
constexpr char kWalletHostName[] = "wallet";

constexpr char kSettingsHostName[] = "settings";

constexpr char kSearchEnginesPath[] = "/searchEngines";
constexpr char kSearchPath[] = "/search";
constexpr char kSearchQuery[] = "search";

}  // namespace

bool HasSearchQuery(const GURL& url) {
  CHECK(url.is_valid());
  CHECK(url.has_query());

  for (net::QueryIterator iter(url); !iter.IsAtEnd(); iter.Advance()) {
    if (iter.GetKey() != kSearchQuery || iter.GetValue().empty()) {
      return false;
    }
  }

  return true;
}

bool ShouldSupportInternalUrl(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  if (!url.SchemeIs(kChromeScheme)) {
    // Do not support schemes other than chrome://.
    return false;
  }

  const std::string host_name = url.host();

  if (host_name == kRewardsHostName || host_name == kSyncHostName ||
      host_name == kWalletHostName) {
    // Support chrome://rewards, chrome://sync, and chrome://wallet hosts.
    return true;
  }

  if (host_name != kSettingsHostName) {
    // Do not support hosts other than chrome://settings.
    return false;
  }

  if (url.path() == kSearchEnginesPath || url.path() == kSearchPath) {
    if (!url.has_query()) {
      // Support chrome://settings/searchEngines and chrome://settings/search
      // paths without a query.
      return true;
    }

    return HasSearchQuery(url);
  }

  // We can reject everything else!
  return false;
}

bool HostHasRegistryControlledDomain(const std::string_view host) {
  return net::registry_controlled_domains::HostHasRegistryControlledDomain(
      host, net::registry_controlled_domains::EXCLUDE_UNKNOWN_REGISTRIES,
      net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool DoesETLDPlusOneContainWildcards(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  // There's no need to check for a question mark wildcard, because
  // `GetDomainAndRegistry` returns an empty string for invalid URLs. Host names
  // containing a '?' are deemed invalid.
  constexpr char kUrlEncodedAsteriskWildcard[] = "%2A";

  const std::string domain_and_registry =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  return base::Contains(domain_and_registry, kUrlEncodedAsteriskWildcard);
}

}  // namespace brave_ads
