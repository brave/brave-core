/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util_internal.h"

#include <string>
#include <string_view>

#include "base/check.h"
#include "base/containers/contains.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr std::string_view kChromeScheme = "chrome";

constexpr std::string_view kGettingStartedHostName = "getting-started";
constexpr std::string_view kRewardsHostName = "rewards";
constexpr std::string_view kSyncHostName = "sync";
constexpr std::string_view kLeoAiHostName = "leo-ai";
constexpr std::string_view kWalletHostName = "wallet";

constexpr std::string_view kSettingsHostName = "settings";

constexpr std::string_view kSurveyPanelistPath = "/surveyPanelist";

constexpr std::string_view kSearchEnginesPath = "/searchEngines";
constexpr std::string_view kSearchPath = "/search";
constexpr std::string_view kDefaultSearchPath = "/search/defaultSearch";
constexpr std::string_view kSearchQuery = "search";

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

  std::string_view url_host = url.host_piece();
  std::string_view url_path = url.path_piece();

  if (url_host == kGettingStartedHostName || url_host == kRewardsHostName ||
      url_host == kSyncHostName || url_host == kLeoAiHostName ||
      url_host == kWalletHostName) {
    // Support chrome://getting-started, chrome://rewards, chrome://sync,
    // chrome://leo-ai, and chrome://wallet hosts only if the path is "/".
    return url_path == "/";
  }

  if (url_host != kSettingsHostName) {
    // Do not support hosts other than chrome://settings.
    return false;
  }

  if (url_path == kSurveyPanelistPath) {
    // Support chrome://settings/surveyPanelist.
    return true;
  }

  if (url_path == kSearchEnginesPath || url_path == kSearchPath ||
      url_path == kDefaultSearchPath) {
    if (!url.has_query()) {
      // Support chrome://settings/searchEngines,
      // chrome://settings/searchEngines/defaultSearch and
      // chrome://settings/search paths without a query.
      return true;
    }

    // Support chrome://settings/search paths with a query.
    return HasSearchQuery(url);
  }

  // We can reject everything else!
  return false;
}

bool HostHasRegistryControlledDomain(std::string_view host) {
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
