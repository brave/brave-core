/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_utils.h"

#include <array>
#include <optional>

#include "base/ranges/algorithm.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace brave_rewards {

namespace {

const std::array kMediaPlatformDomains = {"twitter.com", "github.com",
                                          "reddit.com",  "twitch.tv",
                                          "vimeo.com",   "youtube.com"};

bool IsMediaPlatformURL(const GURL& url) {
  return base::ranges::any_of(kMediaPlatformDomains, [&url](auto* domain) {
    return net::registry_controlled_domains::SameDomainOrHost(
        url, GURL("https://" + std::string(domain)),
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  });
}

}  // namespace

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

bool IsAutoContributeHandledByContentScript(const GURL& url) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  return false;
#else
  return IsMediaPlatformURL(url);
#endif
}

}  // namespace brave_rewards
