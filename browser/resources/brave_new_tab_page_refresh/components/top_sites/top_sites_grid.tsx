/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Icon from '@brave/leo/react/icon'
import NavDots from '@brave/leo/react/navdots'

import { TopSite } from '../../state/top_sites_store'
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
import { GridItem, useTopSitesGridItems } from './top_sites_grid_items'

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
}

export function TopSitesGrid(props: Props) {
  const actions = useTopSitesActions()
  const topSitesCount = useTopSitesState((s) => s.topSites.length)
  const gridItems = useTopSitesGridItems({ canAddSite: props.canAddSite })
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
      tileSelector: 'a.top-site-tile',
      autoScroll: 'horizontal',
    }),
  )

  const pageWidth = columnsPerPage * tileWidth

  const tileCount = gridItems.length

  const pages = React.useMemo(() => {
    return splitIntoPages(gridItems, {
      columnsPerPage,
      rowsPerPage: props.expanded ? maxTileRowCount : collapsedTileRowCount,
    })
  }, [gridItems, columnsPerPage, props.expanded])

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
        // draggable.
        const fromItem = gridItems[from]
        if (!fromItem || fromItem.type !== 'top-site') {
          return
        }

        // A drop at or before a (non-draggable) sponsored site snaps to the
        // start of the top sites list; a drop past the last top site snaps
        // to the end.
        const toItem = gridItems[to]
        let position: number
        if (toItem?.type === 'top-site') {
          position = toItem.index
        } else if (toItem?.type === 'sponsored-site') {
          position = 0
        } else {
          position = topSitesCount - 1
        }
        if (position >= 0) {
          actions.setTopSitePosition(fromItem.site.url, position)
        }
      },
    })
  }, [scrollPage, gridItems, topSitesCount])

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
                  tile.type === 'add-button' ? (
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
                  ) : tile.type === 'sponsored-site' ? (
                    <SponsoredSitesTile
                      key={tile.site.targetUrl}
                      site={tile.site}
                      onContextMenu={props.onSponsoredSiteContextMenu}
                    />
                  ) : (
                    <TopSitesTile
                      key={tile.site.url}
                      topSite={tile.site}
                      canDrag={props.canReorderSites}
                      onContextMenu={contextMenuHandler(tile.site)}
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
export function getColumnsPerPage(maxColumns: number) {
  const available =
    document.body.clientWidth - nonGridWidth - horizontalContentPadding * 2
  let columns = Math.floor(available / tileWidth)
  columns = Math.min(maxColumns, columns)
  columns = Math.max(minTileColumnCount, columns)
  return columns
}

export interface SplitIntoPagesOptions {
  columnsPerPage: number
  rowsPerPage: number
}

export function splitIntoPages(
  items: GridItem[],
  options: SplitIntoPagesOptions,
) {
  const { columnsPerPage, rowsPerPage } = options

  if (columnsPerPage === 0 || rowsPerPage === 0) {
    return []
  }

  let currentRow: GridItem[] = []
  let currentPage: GridItem[][] = [currentRow]
  const pages: GridItem[][][] = [currentPage]

  items.forEach((item) => {
    if (currentRow.length >= columnsPerPage) {
      if (currentPage.length >= rowsPerPage) {
        currentPage = []
        pages.push(currentPage)
      }
      currentRow = []
      currentPage.push(currentRow)
    }
    currentRow.push(item)
  })

  return pages
}

export function useTopSiteAdded(callback: () => void, deps: any[]) {
  const count = useTopSitesState((s) => s.topSites.length)
  const prevRef = React.useRef(count)
  React.useEffect(() => {
    if (count > prevRef.current && prevRef.current > 0) {
      callback()
    }
    prevRef.current = count
  }, [count, callback, ...deps])
}
