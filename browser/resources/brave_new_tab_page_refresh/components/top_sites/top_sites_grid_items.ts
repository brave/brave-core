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
  // A sponsored site is only shown when its advertiser domain also appears
  // among the current top sites, which already reflect whichever list kind
  // is active upstream, Frequently Visited or the user's favourites. Showing
  // one that is not tied to anything the user has visited or chosen would be
  // poor UX.
  const relevantSponsoredSites = sponsoredSitesMatchingTopSites(
    topSites,
    sponsoredSites,
  )
  const deduplicatedTopSites = topSitesWithoutSponsoredSiteDuplicates(
    topSites,
    relevantSponsoredSites,
  )
  const items: GridItem[] = [
    ...relevantSponsoredSites.map(
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

// The exact hostname of a sponsored site's target, with any leading "www."
// stripped so a top site on either the www or non-www variant still matches.
// Returns null for an unparsable `targetUrl` (e.g. malformed ad server data),
// which callers treat as never matching any top site.
function advertiserDomain(sponsoredSite: SponsoredSite): string | null {
  try {
    const hostname = new URL(sponsoredSite.targetUrl).hostname
    return hostname.startsWith('www.') ? hostname.slice(4) : hostname
  } catch {
    return null
  }
}

function sponsoredSitesMatchingTopSites(
  topSites: TopSite[],
  sponsoredSites: SponsoredSite[],
): SponsoredSite[] {
  return sponsoredSites.filter((sponsoredSite) => {
    const domain = advertiserDomain(sponsoredSite)
    return (
      domain !== null
      && topSites.some((topSite) =>
        matchesAdvertiserDomain(topSite.url, domain),
      )
    )
  })
}

export function topSitesWithoutSponsoredSiteDuplicates(
  topSites: TopSite[],
  sponsoredSites: SponsoredSite[],
): TopSite[] {
  if (sponsoredSites.length === 0) {
    return topSites
  }

  // This is intentionally exact: a top site on an unrelated subdomain (e.g.
  // `aws.amazon.com` when the sponsored site targets `amazon.com`) is a
  // distinct destination and should not be deduped away.
  const advertiserDomains = sponsoredSites
    .map(advertiserDomain)
    .filter((domain): domain is string => domain !== null)
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
function matchesAdvertiserDomain(topSiteUrl: string, domain: string) {
  try {
    const hostname = new URL(topSiteUrl).hostname
    return hostname === domain || hostname === `www.${domain}`
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
