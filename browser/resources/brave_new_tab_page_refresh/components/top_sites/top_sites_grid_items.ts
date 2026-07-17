/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { SponsoredSite, TopSite } from '../../state/top_sites_store'
import { useTopSitesState } from '../../context/top_sites_context'

export type GridItem =
  | { type: 'add-button' }
  | { type: 'top-site'; site: TopSite; index: number }
  | { type: 'sponsored-site'; site: SponsoredSite }

export interface UseTopSitesGridItemsOptions {
  canAddSite: boolean
}

// `sponsoredSites` is empty when the user has hidden sponsored sites, so
// callers don't need to check `showSponsoredSites` separately.
export function useTopSitesGridItems(
  options: UseTopSitesGridItemsOptions,
): GridItem[] {
  const topSites = useTopSitesState((s) => s.topSites)
  const sponsoredSites = useTopSitesState((s) =>
    s.showSponsoredSites ? s.sponsoredSites : [],
  )
  return React.useMemo(
    () => buildGridItems(topSites, sponsoredSites, options.canAddSite),
    [topSites, sponsoredSites, options.canAddSite],
  )
}

export function buildGridItems(
  topSites: TopSite[],
  sponsoredSites: SponsoredSite[],
  canAddSite: boolean,
): GridItem[] {
  const deduplicatedTopSites = topSitesWithoutSponsoredSiteDuplicates(
    topSites,
    sponsoredSites,
  )
  const items: GridItem[] = [
    ...sponsoredSites.map(
      (site): GridItem => ({ type: 'sponsored-site', site }),
    ),
    // `index` is the site's position in the full (non-deduplicated) top
    // sites list, so callers can resolve a grid position back to a
    // `setTopSitePosition` argument without a further lookup.
    ...deduplicatedTopSites.map(
      (site): GridItem => ({
        type: 'top-site',
        site,
        index: topSites.indexOf(site),
      }),
    ),
  ]
  if (canAddSite) {
    items.push({ type: 'add-button' })
  }
  return items
}

export function topSitesWithoutSponsoredSiteDuplicates(
  topSites: TopSite[],
  sponsoredSites: SponsoredSite[],
): TopSite[] {
  if (sponsoredSites.length === 0) {
    return topSites
  }

  // The exact hostname of each sponsored site's target, with any leading "www."
  // stripped so a top site on either the www or non-www variant still matches.
  // This is intentionally exact: a top site on an unrelated subdomain (e.g.
  // `aws.amazon.com` when the sponsored site targets `amazon.com`) is a
  // distinct destination and should not be deduped away.
  const advertiserDomains = sponsoredSites.map((sponsoredSite) => {
    const hostname = new URL(sponsoredSite.targetUrl).hostname
    return hostname.startsWith('www.') ? hostname.slice(4) : hostname
  })
  return topSites.filter(
    (topSite) =>
      // A top site with a search query is a specific page the user visited, not
      // a generic bookmark of the advertiser's domain, so it isn't treated as a
      // duplicate of the sponsored site. For example, a top site at
      // https://www.amazon.com/s?k=waldo is kept even when a sponsored site
      // targets `amazon.com`.
      urlHasSearch(topSite.url)
      || !advertiserDomains.some((domain) =>
        matchesAdvertiserDomain(topSite.url, domain),
      ),
  )
}

// Matches only the exact advertiser hostname or its www variant, not subdomains
// or parent domains, to avoid deduping distinct destinations. For example, a
// sponsored site targeting `www.amazon.com` (normalized to `amazon.com` by the
// caller) dedupes a top site at `amazon.com` or `www.amazon.com`, but a
// sponsored site targeting `aws.amazon.com` only dedupes `aws.amazon.com` or
// `www.aws.amazon.com`, not the parent `amazon.com` and not other unrelated
// subdomains.
function matchesAdvertiserDomain(topSiteUrl: string, advertiserDomain: string) {
  try {
    const hostname = new URL(topSiteUrl).hostname
    return (
      hostname === advertiserDomain || hostname === `www.${advertiserDomain}`
    )
  } catch {
    return false
  }
}

function urlHasSearch(url: string) {
  try {
    return !!new URL(url).search
  } catch {
    return false
  }
}
