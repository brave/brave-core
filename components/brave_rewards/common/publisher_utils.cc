/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/publisher_utils.h"

#include <array>
#include <optional>

#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave_rewards {

namespace {

constexpr auto kMediaPlatformDomains =
    std::to_array<std::string_view>({"reddit.com", "twitch.tv", "twitter.com",
                                     "vimeo.com", "x.com", "youtube.com"});

constexpr auto kMediaPlatformPrefixes = std::to_array<std::string_view>(
    {"reddit#", "twitch#", "twitter#", "vimeo#", "youtube#"});

}  // namespace

bool IsMediaPlatformURL(const GURL& url) {
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    return false;
  }
  return base::ranges::any_of(kMediaPlatformDomains, [&url](auto domain) {
    return net::registry_controlled_domains::SameDomainOrHost(
        url, GURL("https://" + std::string(domain)),
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  });
}

std::optional<std::string> GetMediaPlatformFromPublisherId(
    std::string_view publisher_id) {
  for (auto prefix : kMediaPlatformPrefixes) {
    CHECK(prefix.length() > 1);
    if (publisher_id.starts_with(prefix)) {
      return std::string(publisher_id.substr(0, prefix.length() - 1));
    }
  }
  return std::nullopt;
}

std::optional<std::string> GetPublisherIdFromURL(const GURL& url) {
  if (IsMediaPlatformURL(url)) {
    return std::nullopt;
  }
  return GetPublisherDomainFromURL(url);
}

std::optional<std::string> GetPublisherDomainFromURL(const GURL& url) {
  if (!url.SchemeIsHTTPOrHTTPS()) {
    return std::nullopt;
  }

  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (domain.empty()) {
    return std::nullopt;
  }
  return domain;
}

}  // namespace brave_rewards
