/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, render, renderHook } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { TopSitesContext } from '../../context/top_sites_context'
import {
  SponsoredSite,
  TopSite,
  TopSitesListKind,
  TopSitesState,
} from '../../state/top_sites_store'
import { GridItem } from './top_sites_grid_items'
import { maxTileColumnCount, minTileColumnCount } from './top_sites.style'
import {
  getColumnsPerPage,
  splitIntoPages,
  TopSitesGrid,
  useTopSiteAdded,
} from './top_sites_grid'

let dragCallbacks: {
  onDrop?: (dragFrom: number, dragTo: number) => void
} = {}

jest.mock('./tile_drag_handler', () => ({
  createTileDragHandler: () => ({
    observe: () => () => {},
    setCallbacks: (callbacks: typeof dragCallbacks) => {
      dragCallbacks = callbacks
    },
  }),
}))

if (typeof ResizeObserver === 'undefined') {
  ;(global as any).ResizeObserver = class {
    observe() {}
    unobserve() {}
    disconnect() {}
  }
}

if (!Element.prototype.scrollTo) {
  Element.prototype.scrollTo = () => {}
}

function createTopSite(url: string): TopSite {
  return { title: 'Site', url, favicon: '' }
}

function createSponsoredSite(targetUrl: string): SponsoredSite {
  return {
    relativeImageUrl: '',
    title: 'foo',
    adDisclosure: 'bar',
    targetUrl,
  }
}

function createStore(
  state: Partial<TopSitesState>,
  actions: Partial<TopSitesState['actions']> = {},
) {
  return createStateStore<TopSitesState>({
    initialized: true,
    maxCustomTopSites: 48,
    showSponsoredSites: true,
    showTopSites: true,
    topSitesListKind: TopSitesListKind.kMostVisited,
    sponsoredSites: [],
    topSites: [],
    actions: {
      setShowSponsoredSites() {},
      setShowTopSites() {},
      setTopSitesListKind() {},
      addTopSite() {},
      updateTopSite() {},
      removeTopSite() {},
      undoRemoveTopSite() {},
      setTopSitePosition() {},
      recordTopSiteClick() {},
      ...actions,
    },
    ...state,
  })
}

function createWrapper(store: ReturnType<typeof createStore>) {
  return function Wrapper({ children }: { children: React.ReactNode }) {
    return (
      <TopSitesContext.Provider value={store}>
        {children}
      </TopSitesContext.Provider>
    )
  }
}

function renderGrid(
  state: Partial<TopSitesState>,
  actions: Partial<TopSitesState['actions']> = {},
) {
  const store = createStore(state, actions)
  render(
    <TopSitesContext.Provider value={store}>
      <TopSitesGrid
        expanded={false}
        canAddSite={false}
        canReorderSites
        onAddTopSite={() => {}}
        onTopSiteContextMenu={() => {}}
        onSponsoredSiteContextMenu={() => {}}
      />
    </TopSitesContext.Provider>,
  )
}

describe('getColumnsPerPage', () => {
  it.each([
    [
      'should clamp to the minimum column count when the viewport is too narrow',
      0,
      minTileColumnCount,
    ],
    [
      'should clamp to the maximum column count when the viewport is very wide',
      10000,
      maxTileColumnCount,
    ],
  ])('%s', (_name: string, clientWidth: number, expectedColumns: number) => {
    Object.defineProperty(document.body, 'clientWidth', {
      value: clientWidth,
      configurable: true,
    })
    expect(getColumnsPerPage(maxTileColumnCount)).toBe(expectedColumns)
  })
})

function topSiteItem(site: TopSite, index: number): GridItem {
  return { type: 'top-site', site, index }
}

function sponsoredSiteItem(site: SponsoredSite): GridItem {
  return { type: 'sponsored-site', site }
}

const addButtonItem: GridItem = { type: 'add-button' }

describe('splitIntoPages', () => {
  it.each([
    ['no columns', 0, 1],
    ['no rows', 1, 0],
  ])(
    'should return no pages when the grid has %s',
    (_description: string, columnsPerPage: number, rowsPerPage: number) => {
      const items = [topSiteItem(createTopSite('https://foo.com'), 0)]
      expect(
        splitIntoPages(items, { columnsPerPage, rowsPerPage }),
      ).toHaveLength(0)
    },
  )

  it('should return a single empty page when there are no items', () => {
    expect(splitIntoPages([], { columnsPerPage: 2, rowsPerPage: 1 })).toEqual([
      [[]],
    ])
  })

  it('should place a single sponsored site alone when there are no top sites', () => {
    const sponsoredSite = sponsoredSiteItem(
      createSponsoredSite('https://bar.com'),
    )
    const pages = splitIntoPages([sponsoredSite], {
      columnsPerPage: 2,
      rowsPerPage: 1,
    })
    expect(pages).toEqual([[[sponsoredSite]]])
  })

  it('should place multiple sponsored sites, in order, when there are no top sites', () => {
    const sponsoredSites = [
      sponsoredSiteItem(createSponsoredSite('https://bar.com')),
      sponsoredSiteItem(createSponsoredSite('https://baz.com')),
    ]
    const pages = splitIntoPages(sponsoredSites, {
      columnsPerPage: 3,
      rowsPerPage: 1,
    })
    expect(pages).toEqual([[sponsoredSites]])
  })

  it('should place a single sponsored site ahead of a single top site', () => {
    const topSite = topSiteItem(createTopSite('https://foo.com'), 0)
    const sponsoredSite = sponsoredSiteItem(
      createSponsoredSite('https://bar.com'),
    )
    const pages = splitIntoPages([sponsoredSite, topSite], {
      columnsPerPage: 2,
      rowsPerPage: 1,
    })
    expect(pages).toEqual([[[sponsoredSite, topSite]]])
  })

  it('should show an add-site button as the last tile when adding a site is allowed', () => {
    const topSite = topSiteItem(createTopSite('https://foo.com'), 0)
    const pages = splitIntoPages([topSite, addButtonItem], {
      columnsPerPage: 2,
      rowsPerPage: 1,
    })
    expect(pages).toEqual([[[topSite, addButtonItem]]])
  })

  it('should start a new row once the current row is full', () => {
    const topSiteA = topSiteItem(createTopSite('https://qux.com'), 0)
    const topSiteB = topSiteItem(createTopSite('https://quux.com'), 1)
    const topSiteC = topSiteItem(createTopSite('https://corge.com'), 2)
    const pages = splitIntoPages([topSiteA, topSiteB, topSiteC], {
      columnsPerPage: 2,
      rowsPerPage: 2,
    })
    expect(pages).toEqual([[[topSiteA, topSiteB], [topSiteC]]])
  })

  it('should start a new page once the current page is full', () => {
    const topSiteA = topSiteItem(createTopSite('https://qux.com'), 0)
    const topSiteB = topSiteItem(createTopSite('https://quux.com'), 1)
    const topSiteC = topSiteItem(createTopSite('https://corge.com'), 2)
    const pages = splitIntoPages([topSiteA, topSiteB, topSiteC], {
      columnsPerPage: 1,
      rowsPerPage: 1,
    })
    expect(pages).toEqual([[[topSiteA]], [[topSiteB]], [[topSiteC]]])
  })
})

describe('useTopSiteAdded', () => {
  it.each([
    [
      'should notify when a top site is added after the initial list',
      [createTopSite('https://foo.com')],
      1,
    ],
    [
      'should not notify when the first top site is added to an empty list',
      [],
      0,
    ],
  ])(
    '%s',
    async (
      _name: string,
      initialTopSites: TopSite[],
      expectedCalls: number,
    ) => {
      const store = createStore({ topSites: initialTopSites })
      const callback = jest.fn()
      renderHook(() => useTopSiteAdded(callback, []), {
        wrapper: createWrapper(store),
      })

      await act(async () => {
        store.update({
          topSites: [...initialTopSites, createTopSite('https://bar.com')],
        })
      })

      expect(callback).toHaveBeenCalledTimes(expectedCalls)
    },
  )
})

describe('TopSitesGrid onDrop', () => {
  beforeEach(() => {
    dragCallbacks = {}
  })

  it('should reorder to the dropped top site position', () => {
    const topSiteA = createTopSite('https://foo.com')
    const topSiteB = createTopSite('https://bar.com')
    const topSiteC = createTopSite('https://baz.com')
    const setTopSitePosition = jest.fn()
    renderGrid(
      { topSites: [topSiteA, topSiteB, topSiteC] },
      { setTopSitePosition },
    )

    dragCallbacks.onDrop?.(0, 2)

    expect(setTopSitePosition).toHaveBeenCalledWith(topSiteA.url, 2)
  })

  it('should snap to the start when dropped on a sponsored site', () => {
    const topSiteA = createTopSite('https://foo.com')
    const topSiteB = createTopSite('https://bar.com')
    const sponsoredSite = createSponsoredSite('https://baz.com')
    const setTopSitePosition = jest.fn()
    renderGrid(
      {
        topSites: [topSiteA, topSiteB],
        sponsoredSites: [sponsoredSite],
      },
      { setTopSitePosition },
    )

    // Grid order is [sponsoredSite, topSiteA, topSiteB]; drag topSiteB (index
    // 2) onto the sponsored site slot (index 0).
    dragCallbacks.onDrop?.(2, 0)

    expect(setTopSitePosition).toHaveBeenCalledWith(topSiteB.url, 0)
  })

  it('should snap to the end when dropped past the last top site', () => {
    const topSiteA = createTopSite('https://foo.com')
    const topSiteB = createTopSite('https://bar.com')
    const topSiteC = createTopSite('https://baz.com')
    const setTopSitePosition = jest.fn()
    renderGrid(
      { topSites: [topSiteA, topSiteB, topSiteC] },
      { setTopSitePosition },
    )

    // Index 3 is past the last grid tile (indices 0-2).
    dragCallbacks.onDrop?.(0, 3)

    expect(setTopSitePosition).toHaveBeenCalledWith(topSiteA.url, 2)
  })

  it('should not reorder when the dragged tile is a sponsored site', () => {
    const topSiteA = createTopSite('https://foo.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const setTopSitePosition = jest.fn()
    renderGrid(
      {
        topSites: [topSiteA],
        sponsoredSites: [sponsoredSite],
      },
      { setTopSitePosition },
    )

    // Grid order is [sponsoredSite, topSiteA]; drag the sponsored site.
    dragCallbacks.onDrop?.(0, 1)

    expect(setTopSitePosition).not.toHaveBeenCalled()
  })
})
