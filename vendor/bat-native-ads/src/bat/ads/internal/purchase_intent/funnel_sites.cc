/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/purchase_intent/funnel_sites.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#include "url/gurl.h"

namespace ads {

FunnelSites::FunnelSites() = default;
FunnelSites::~FunnelSites() = default;

FunnelSiteInfo FunnelSites::GetFunnelSite(
    const std::string& url) {
  const GURL visited_url = GURL(url);
  FunnelSiteInfo funnel_site_info;

  if (!visited_url.has_host()) {
    return funnel_site_info;
  }

  // TODO(Moritz Haller): User map since faster than iterating
  // `_automotive_funnel_sites`
  for (const auto& funnel_site : _automotive_funnel_sites) {
    const GURL funnel_site_url = GURL(funnel_site.url_netloc);

    if (!funnel_site_url.is_valid()) {
      continue;
    }

    if (SameDomainOrHost(visited_url, funnel_site_url,
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      funnel_site_info = funnel_site;
      return funnel_site_info;
    }
  }

  return funnel_site_info;
}

}  // namespace ads
