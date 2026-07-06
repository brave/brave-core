/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import NavDots from '@brave/leo/react/navdots'

import { SponsoredSite, TopSite } from '../../state/top_sites_store'
import {
  useTopSitesState,
  useTopSitesActions,
} from '../../context/top_sites_context'
import { getString } from '../../lib/strings'
import { usePersistedJSON } from '$web-common/usePersistedState'
import { TopSitesTile } from './top_site_tile'
import { SponsoredSitesTile } from './sponsored_site_tile'
import { createTileDragHandler } from './tile_drag_handler'
import { inlineCSSVars } from '../../lib/inline_css_vars'

import { horizontalContentPadding } from '../app.style'

import {
  tileWidth,
  collapsedTileColumnCount,
  collapsedTileRowCount,
  nonGridWidth,
  maxTileColumnCount,
  minTileColumnCount,
  maxTileRowCount,
} from './top_sites.style'

interface Props {
  expanded: boolean
  canAddSite: boolean
  canReorderSites: boolean
  onAddTopSite: () => void
  onTopSiteContextMenu: (topSite: TopSite, event: React.MouseEvent) => void
  onSponsoredSiteContextMenu: (event: React.MouseEvent) => void
  suppressSponsoredTooltip: boolean
}

export function TopSitesGrid(props: Props) {
  const actions = useTopSitesActions()
  const topSites = useTopSitesState((s) => s.topSites)
  const sponsoredSites = useTopSitesState((s) =>
    s.showSponsoredSites ? s.sponsoredSites : [],
  )
  const columnsPerPage = useColumnsPerPage(
    props.expanded ? maxTileColumnCount : collapsedTileColumnCount,
  )
  const scrollRef = React.useRef<HTMLDivElement>(null)

  const [scrollPage, setScrollPage] = usePersistedJSON(
    'ntp-top-sites-page',
    (data) => Number(data) || 0,
  )

  const [dragHandler] = React.useState(() =>
    createTileDragHandler({
      // Scoped to actual tile links only: SponsoredSitesTile also renders a
      // "Learn more" <a> inside its tooltip content, which would otherwise
      // be matched too and throw off the from/to indices below.
      tileSelector: 'a.top-site-tile',
      autoScroll: 'horizontal',
    }),
  )

  const pageWidth = columnsPerPage * tileWidth

  const deduplicatedTopSites = React.useMemo(
    () => topSitesWithoutSponsoredSiteDuplicates(topSites, sponsoredSites),
    [topSites, sponsoredSites],
  )

  const tileCount =
    deduplicatedTopSites.length
    + sponsoredSites.length
    + (props.canAddSite ? 1 : 0)

  const pages = React.useMemo(() => {
    return splitIntoPages(deduplicatedTopSites, {
      columnsPerPage,
      rowsPerPage: props.expanded ? maxTileRowCount : collapsedTileRowCount,
      canAddSite: props.canAddSite,
      sponsoredSites,
    })
  }, [
    deduplicatedTopSites,
    columnsPerPage,
    props.canAddSite,
    props.expanded,
    sponsoredSites,
  ])

  React.useEffect(() => {
    const elem = scrollRef.current
    if (!elem) {
      return
    }
    return dragHandler.observe(elem)
  }, [dragHandler])

  React.useEffect(() => {
    scrollToPage(scrollPage, 'instant')
  }, [])

  React.useEffect(() => {
    dragHandler.setCallbacks({
      onScroll(direction) {
        scrollToPage(scrollPage + (direction === 'forward' ? 1 : -1))
      },
      onDrop(from, to) {
        // `from`/`to` are indices over every rendered <a> tile, including
        // sponsored sites that are rendered ahead of top sites and are not
        // draggable. Offset past them, then resolve the drop target back to
        // its real position in `topSites` (not `deduplicatedTopSites`, which
        // may have fewer entries than `topSites` due to advertiser-domain
        // deduplication).
        const site = deduplicatedTopSites[from - sponsoredSites.length]
        if (!site) {
          return
        }
        // A drop at or before the (non-draggable) sponsored sites snaps to
        // the start of the top sites list; a drop past the last top site
        // snaps to the end.
        let pos: number
        if (to < sponsoredSites.length) {
          pos = 0
        } else {
          const targetSite = deduplicatedTopSites[to - sponsoredSites.length]
          pos = targetSite ? topSites.indexOf(targetSite) : topSites.length - 1
        }
        if (pos >= 0) {
          actions.setTopSitePosition(site.url, pos)
        }
      },
    })
  }, [scrollPage, topSites, deduplicatedTopSites, sponsoredSites])

  useTopSiteAdded(() => {
    const elem = scrollRef.current
    if (elem && props.canAddSite) {
      elem.scrollTo({ left: elem.scrollWidth, behavior: 'smooth' })
    }
  }, [props.canAddSite])

  function onScroll() {
    const scrollLeft = scrollRef.current?.scrollLeft ?? 0
    setScrollPage(Math.round(scrollLeft / pageWidth))
  }

  function scrollToPage(page: number, scrollBehavior?: ScrollBehavior) {
    if (page < 0) {
      page = 0
    } else if (page >= pages.length) {
      page = pages.length - 1
    }
    scrollRef.current?.scrollTo({
      left: page * pageWidth,
      behavior: scrollBehavior ?? 'smooth',
    })
  }

  function contextMenuHandler(topSite: TopSite) {
    return (event: React.MouseEvent) => {
      props.onTopSiteContextMenu(topSite, event)
    }
  }

  return (
    <div>
      <div
        ref={scrollRef}
        className='top-site-tiles-mask'
        onScroll={onScroll}
        style={inlineCSSVars({
          '--self-columns-per-page': Math.min(columnsPerPage, tileCount),
        })}
      >
        {pages.map((page, i) => (
          <div
            key={i}
            className='top-site-tiles'
          >
            {page.map((row, i) => (
              <div
                key={i}
                className='top-site-row'
              >
                {row.map((tile, i) =>
                  tile === 'add-button' ? (
                    <button
                      key='add-button'
                      className='top-site-tile'
                      onClick={props.onAddTopSite}
                    >
                      <span className='top-site-icon'>
                        <Icon name='plus-add' />
                      </span>
                      <span className='top-site-title'>
                        {getString(S.NEW_TAB_ADD_TOP_SITE_LABEL)}
                      </span>
                    </button>
                  ) : 'targetUrl' in tile ? (
                    <SponsoredSitesTile
                      key={tile.targetUrl.url}
                      site={tile}
                      suppressTooltip={props.suppressSponsoredTooltip}
                      onContextMenu={props.onSponsoredSiteContextMenu}
                    />
                  ) : (
                    <TopSitesTile
                      key={tile.url}
                      topSite={tile}
                      canDrag={props.canReorderSites}
                      onContextMenu={contextMenuHandler(tile)}
                      onNavigate={actions.recordTopSiteClick}
                    />
                  ),
                )}
              </div>
            ))}
          </div>
        ))}
      </div>
      {pages.length > 1 && (
        <div className='page-nav'>
          <NavDots
            dotCount={pages.length}
            activeDot={scrollPage}
            onChange={(event) => scrollToPage(event.activeDot)}
          />
        </div>
      )}
    </div>
  )
}

function useColumnsPerPage(maxColumns: number) {
  const [columns, setColumns] = React.useState(getColumnsPerPage(maxColumns))
  React.useEffect(() => {
    const observer = new ResizeObserver(() => {
      setColumns(getColumnsPerPage(maxColumns))
    })
    observer.observe(document.body)
    return () => observer.disconnect()
  }, [maxColumns])
  return columns
}

// In order to avoid a layout dependency on the dimensions of the
// not-yet-rendered top sites container, calculate the available grid width
// based upon the current body width, minus non-grid width.
function getColumnsPerPage(maxColumns: number) {
  const available =
    document.body.clientWidth - nonGridWidth - horizontalContentPadding * 2
  let columns = Math.floor(available / tileWidth)
  columns = Math.min(maxColumns, columns)
  columns = Math.max(minTileColumnCount, columns)
  return columns
}

interface SplitIntoPagesOptions {
  columnsPerPage: number
  rowsPerPage: number
  canAddSite: boolean
  sponsoredSites: SponsoredSite[]
}

type GridItem = TopSite | SponsoredSite | 'add-button'

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
  const advertiserDomains = sponsoredSites.map((tile) => {
    const hostname = new URL(tile.targetUrl.url).hostname
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

function splitIntoPages(topSites: TopSite[], options: SplitIntoPagesOptions) {
  const { columnsPerPage, rowsPerPage, canAddSite, sponsoredSites } = options

  if (columnsPerPage === 0 || rowsPerPage === 0) {
    return []
  }

  const tiles: GridItem[] = [...sponsoredSites, ...topSites]
  if (canAddSite) {
    tiles.push('add-button')
  }

  let currentRow: GridItem[] = []
  let currentPage: GridItem[][] = [currentRow]
  const pages: GridItem[][][] = [currentPage]

  tiles.forEach((tile) => {
    if (currentRow.length >= columnsPerPage) {
      if (currentPage.length >= rowsPerPage) {
        currentPage = []
        pages.push(currentPage)
      }
      currentRow = []
      currentPage.push(currentRow)
    }
    currentRow.push(tile)
  })

  return pages
}

function useTopSiteAdded(callback: () => void, deps: any[]) {
  const count = useTopSitesState((s) => s.topSites.length)
  const prevRef = React.useRef(count)
  React.useEffect(() => {
    if (count > prevRef.current && prevRef.current > 0) {
      callback()
    }
    prevRef.current = count
  }, [count, callback, ...deps])
}
