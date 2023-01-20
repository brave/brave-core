/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/publisher_utils.h"

#include <array>

#include "base/ranges/algorithm.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

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

absl::optional<std::string> GetPublisherIdFromURL(const GURL& url) {
  if (IsMediaPlatformURL(url)) {
    return absl::nullopt;
  }
  return GetPublisherDomainFromURL(url);
}

absl::optional<std::string> GetPublisherDomainFromURL(const GURL& url) {
#if BUILDFLAG(ENABLE_IPFS)
  if (url.SchemeIs(ipfs::kIPNSScheme)) {
    std::string domain = ipfs::GetRegistryDomainFromIPNS(url);
    if (domain.empty()) {
      return absl::nullopt;
    }
    return domain;
  }
#endif

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return absl::nullopt;
  }

  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  if (domain.empty()) {
    return absl::nullopt;
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
