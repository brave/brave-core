/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/url_util.h"

#include "base/ranges/algorithm.h"
#include "base/strings/pattern.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_ads {

namespace {

constexpr char kBraveScheme[] = "brave";
constexpr char kChromeScheme[] = "chrome";

constexpr char kRewardsHostName[] = "rewards";
constexpr char kSyncHostName[] = "sync";
constexpr char kWalletHostName[] = "wallet";

constexpr char kSettingsHostName[] = "settings";

constexpr char kSearchEnginesPath[] = "/searchEngines";
constexpr char kSearchPath[] = "/search";
constexpr char kSearchQuery[] = "search";

GURL ReplaceUrlBraveHostWithChromeHost(const GURL& url) {
  CHECK(url.is_valid());

  if (!url.SchemeIs(kBraveScheme)) {
    return url;
  }

  GURL::Replacements replacements;
  replacements.SetSchemeStr(kChromeScheme);
  return url.ReplaceComponents(replacements);
}

}  // namespace

GURL GetUrlWithEmptyQuery(const GURL& url) {
  CHECK(url.is_valid());

  GURL::Replacements replacements;
  replacements.ClearQuery();
  return url.ReplaceComponents(replacements);
}

bool DoesSupportUrl(const GURL& url) {
  if (!url.is_valid()) {
    return false;
  }

  if (url.SchemeIs(url::kHttpsScheme)) {
    // Always support https:// scheme.
    return true;
  }

  // We must replace the brave:// scheme with chrome:// due to GURL not parsing
  // brave:// schemes.
  const GURL modified_url = ReplaceUrlBraveHostWithChromeHost(url);
  if (!modified_url.SchemeIs(kChromeScheme)) {
    return false;
  }

  const std::string host_name = modified_url.host();
  if (host_name == kRewardsHostName || host_name == kSyncHostName ||
      host_name == kWalletHostName) {
    // Support chrome://rewards, chrome://sync and chrome://wallet URLs.
    return true;
  }

  if (host_name == kSettingsHostName) {
    if (modified_url.path() == kSearchEnginesPath ||
        modified_url.path() == kSearchPath) {
      if (!modified_url.has_query()) {
        // Support chrome://settings/searchEngines and
        // chrome://settings/search URLs.
        return true;
      }

      bool is_supported = true;
      for (net::QueryIterator iter(modified_url); !iter.IsAtEnd();
           iter.Advance()) {
        if (iter.GetKey() != kSearchQuery || iter.GetValue().empty()) {
          is_supported = false;
          break;
        }
      }

      if (is_supported) {
        // Support chrome://settings/searchEngines?search=foobar and
        // chrome://settings/search?search=foobar URLs.
        return true;
      }
    }
  }

  return false;
}

bool MatchUrlPattern(const GURL& url, const std::string& pattern) {
  if (!url.is_valid() || pattern.empty()) {
    return false;
  }

  return base::MatchPattern(url.spec(), pattern);
}

bool MatchUrlPattern(const std::vector<GURL>& urls,
                     const std::string& pattern) {
  if (urls.empty() || pattern.empty()) {
    return false;
  }

  const auto iter = base::ranges::find_if(urls, [&pattern](const GURL& url) {
    return MatchUrlPattern(url, pattern);
  });

  return iter != urls.cend();
}

bool SameDomainOrHost(const GURL& lhs, const GURL& rhs) {
  return net::registry_controlled_domains::SameDomainOrHost(
      lhs, rhs, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool DomainOrHostExists(const std::vector<GURL>& urls, const GURL& url) {
  if (urls.empty()) {
    return false;
  }

  return base::ranges::any_of(
      urls, [&url](const GURL& item) { return SameDomainOrHost(item, url); });
}

}  // namespace brave_ads
