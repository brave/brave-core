/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_tiles/brave_popular_sites_impl.h"

namespace {

// static global variable is used because |sections()| returns const ref value.
std::map<ntp_tiles::SectionType, ntp_tiles::PopularSites::SitesVector>
    g_filtered_sections;

bool ShouldHideSiteFromPopularSites(const ntp_tiles::PopularSites::Site& site) {
  if (site.url == "https://m.youtube.com/")
    return true;

  return false;
}

}  // namespace

namespace ntp_tiles {

// Only PERSONALIZED section type is included in |PopularSitesImpl::sections_|.
// See PopularSitesImpl::ParseVersion6OrAbove() or ParseVersion5().
const std::map<SectionType, PopularSitesImpl::SitesVector>&
BravePopularSitesImpl::sections() const {
  PopularSites::SitesVector filtered_sites;
  const auto iter =
      PopularSitesImpl::sections().find(SectionType::PERSONALIZED);
  if (iter != PopularSitesImpl::sections().end()) {
    const PopularSites::SitesVector& popular_sites = iter->second;
    for (const auto& site : popular_sites) {
      if (ShouldHideSiteFromPopularSites(site))
        continue;
      filtered_sites.push_back(site);
    }
  }
  g_filtered_sections[SectionType::PERSONALIZED] = filtered_sites;
  return g_filtered_sections;
}

}  // namespace ntp_tiles
