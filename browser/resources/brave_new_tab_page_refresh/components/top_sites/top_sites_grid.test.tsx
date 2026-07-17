/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, renderHook } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { TopSitesContext } from '../../context/top_sites_context'
import {
  SponsoredSite,
  TopSite,
  TopSitesListKind,
  TopSitesState,
} from '../../state/top_sites_store'
import { maxTileColumnCount, minTileColumnCount } from './top_sites.style'
import {
  getColumnsPerPage,
  topSitesWithoutSponsoredSiteDuplicates,
  useDeduplicatedTopSites,
  splitIntoPages,
  useTopSiteAdded,
} from './top_sites_grid'

function createTopSite(url: string): TopSite {
  return { title: 'Site', url, favicon: '' }
}

function createSponsoredSite(targetUrl: string): SponsoredSite {
  return {
    relativeImageUrlSpec: '',
    title: 'foo',
    adDisclosure: 'bar',
    targetUrl: { url: targetUrl },
  }
}

function createStore(state: Partial<TopSitesState>) {
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

describe('topSitesWithoutSponsoredSiteDuplicates', () => {
  it('should return an empty list when there are no top sites', () => {
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    expect(
      topSitesWithoutSponsoredSiteDuplicates([], sponsoredSites),
    ).toHaveLength(0)
  })

  it('should return the top sites unchanged when there are no sponsored sites', () => {
    const topSites = [
      createTopSite('https://foo.com'),
      createTopSite('https://baz.com'),
    ]
    expect(topSitesWithoutSponsoredSiteDuplicates(topSites, [])).toBe(topSites)
  })

  it.each([
    ['should keep an unrelated subdomain', 'https://foo.bar.com', false],
    [
      'should remove the www variant of the domain',
      'https://www.bar.com',
      true,
    ],
    [
      'should remove the non-www variant of the domain',
      'https://bar.com',
      true,
    ],
  ])('%s', (_name: string, url: string, isDuplicate: boolean) => {
    const topSite = createTopSite(url)
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    expect(
      topSitesWithoutSponsoredSiteDuplicates([topSite], sponsoredSites),
    ).toEqual(isDuplicate ? [] : [topSite])
  })

  it('should keep a top site with an unparsable URL', () => {
    const topSites = [createTopSite('not-a-url')]
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    expect(
      topSitesWithoutSponsoredSiteDuplicates(topSites, sponsoredSites),
    ).toEqual(topSites)
  })

  it('should keep a top site with a search query even when the domain matches', () => {
    const topSites = [createTopSite('https://www.bar.com/s?k=waldo')]
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    expect(
      topSitesWithoutSponsoredSiteDuplicates(topSites, sponsoredSites),
    ).toEqual(topSites)
  })

  it('should handle multiple sponsored sites and top sites independently', () => {
    const expectedTopSite = createTopSite('https://qux.com')
    const topSites = [
      createTopSite('https://bar.com'),
      expectedTopSite,
      createTopSite('https://foo.com'),
    ]
    const sponsoredSites = [
      createSponsoredSite('https://bar.com'),
      createSponsoredSite('https://foo.com'),
    ]
    expect(
      topSitesWithoutSponsoredSiteDuplicates(topSites, sponsoredSites),
    ).toEqual([expectedTopSite])
  })
})

describe('useDeduplicatedTopSites', () => {
  it('should return empty lists when there are no top sites and no sponsored sites', () => {
    const store = createStore({ topSites: [], sponsoredSites: [] })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toEqual({
      topSites: [],
      sponsoredSites: [],
      deduplicatedTopSites: [],
    })
  })

  it('should return the top sites unchanged when there are no sponsored sites', () => {
    const topSites = [
      createTopSite('https://foo.com'),
      createTopSite('https://qux.com'),
    ]
    const store = createStore({ topSites, sponsoredSites: [] })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toEqual({
      topSites,
      sponsoredSites: [],
      deduplicatedTopSites: topSites,
    })
  })

  it('should return no deduplicated top sites when there are no top sites', () => {
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    const store = createStore({ topSites: [], sponsoredSites })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toEqual({
      topSites: [],
      sponsoredSites,
      deduplicatedTopSites: [],
    })
  })

  it('should expose the visited sites, sponsored sites, and the deduplicated list together', () => {
    const expectedTopSite = createTopSite('https://qux.com')
    const topSites = [createTopSite('https://bar.com'), expectedTopSite]
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    const store = createStore({
      topSites,
      sponsoredSites,
      showSponsoredSites: true,
    })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toEqual({
      topSites,
      sponsoredSites,
      deduplicatedTopSites: [expectedTopSite],
    })
  })

  it('should deduplicate against multiple sponsored sites', () => {
    const expectedTopSite = createTopSite('https://qux.com')
    const topSites = [
      createTopSite('https://bar.com'),
      expectedTopSite,
      createTopSite('https://foo.com'),
    ]
    const sponsoredSites = [
      createSponsoredSite('https://bar.com'),
      createSponsoredSite('https://foo.com'),
    ]
    const store = createStore({
      topSites,
      sponsoredSites,
      showSponsoredSites: true,
    })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current.deduplicatedTopSites).toEqual([expectedTopSite])
  })

  it('should hide sponsored sites when the user has turned them off', () => {
    const topSites = [createTopSite('https://bar.com')]
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    const store = createStore({
      topSites,
      sponsoredSites,
      showSponsoredSites: false,
    })

    const { result } = renderHook(() => useDeduplicatedTopSites(), {
      wrapper: createWrapper(store),
    })

    expect(result.current).toEqual({
      topSites,
      sponsoredSites: [],
      deduplicatedTopSites: topSites,
    })
  })
})

describe('splitIntoPages', () => {
  it.each([
    ['no columns', 0, 1],
    ['no rows', 1, 0],
  ])(
    'should return no pages when the grid has %s',
    (_description: string, columnsPerPage: number, rowsPerPage: number) => {
      const topSites = [createTopSite('https://foo.com')]
      expect(
        splitIntoPages(topSites, {
          columnsPerPage,
          rowsPerPage,
          canAddSite: false,
          sponsoredSites: [],
        }),
      ).toHaveLength(0)
    },
  )

  it('should return a single empty page when there are no top sites and no sponsored sites', () => {
    expect(
      splitIntoPages([], {
        columnsPerPage: 2,
        rowsPerPage: 1,
        canAddSite: false,
        sponsoredSites: [],
      }),
    ).toEqual([[[]]])
  })

  it('should place a single sponsored site alone when there are no top sites', () => {
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const pages = splitIntoPages([], {
      columnsPerPage: 2,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [sponsoredSite],
    })
    expect(pages).toEqual([[[sponsoredSite]]])
  })

  it('should place multiple sponsored sites, in order, when there are no top sites', () => {
    const sponsoredSites = [
      createSponsoredSite('https://bar.com'),
      createSponsoredSite('https://baz.com'),
    ]
    const pages = splitIntoPages([], {
      columnsPerPage: 3,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites,
    })
    expect(pages).toEqual([[sponsoredSites]])
  })

  it('should place a single sponsored site ahead of a single top site', () => {
    const topSite = createTopSite('https://foo.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const pages = splitIntoPages([topSite], {
      columnsPerPage: 2,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [sponsoredSite],
    })
    expect(pages).toEqual([[[sponsoredSite, topSite]]])
  })

  it('should place a single sponsored site ahead of multiple top sites', () => {
    const topSiteA = createTopSite('https://qux.com')
    const topSiteB = createTopSite('https://quux.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const pages = splitIntoPages([topSiteA, topSiteB], {
      columnsPerPage: 3,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [sponsoredSite],
    })
    expect(pages).toEqual([[[sponsoredSite, topSiteA, topSiteB]]])
  })

  it('should place multiple sponsored sites ahead of top sites, in order', () => {
    const topSite = createTopSite('https://foo.com')
    const sponsoredSites = [
      createSponsoredSite('https://bar.com'),
      createSponsoredSite('https://baz.com'),
    ]
    const pages = splitIntoPages([topSite], {
      columnsPerPage: 3,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites,
    })
    expect(pages).toEqual([[[...sponsoredSites, topSite]]])
  })

  it('should only dedupe if related, keeping both tiles for an unrelated subdomain', () => {
    const topSite = createTopSite('https://aws.bar.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const deduplicatedTopSites = topSitesWithoutSponsoredSiteDuplicates(
      [topSite],
      [sponsoredSite],
    )
    const pages = splitIntoPages(deduplicatedTopSites, {
      columnsPerPage: 2,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [sponsoredSite],
    })
    expect(pages).toEqual([[[sponsoredSite, topSite]]])
  })

  it('should show an add-site button as the last tile when adding a site is allowed', () => {
    const topSite = createTopSite('https://foo.com')
    const pages = splitIntoPages([topSite], {
      columnsPerPage: 2,
      rowsPerPage: 1,
      canAddSite: true,
      sponsoredSites: [],
    })
    expect(pages).toEqual([[[topSite, 'add-button']]])
  })

  it('should not show an add-site button when adding a site is not allowed', () => {
    const topSite = createTopSite('https://foo.com')
    const pages = splitIntoPages([topSite], {
      columnsPerPage: 2,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [],
    })
    expect(pages).toEqual([[[topSite]]])
  })

  it('should start a new row once the current row is full', () => {
    const topSiteA = createTopSite('https://qux.com')
    const topSiteB = createTopSite('https://quux.com')
    const topSiteC = createTopSite('https://corge.com')
    const pages = splitIntoPages([topSiteA, topSiteB, topSiteC], {
      columnsPerPage: 2,
      rowsPerPage: 2,
      canAddSite: false,
      sponsoredSites: [],
    })
    expect(pages).toEqual([[[topSiteA, topSiteB], [topSiteC]]])
  })

  it('should start a new page once the current page is full', () => {
    const topSiteA = createTopSite('https://qux.com')
    const topSiteB = createTopSite('https://quux.com')
    const topSiteC = createTopSite('https://corge.com')
    const pages = splitIntoPages([topSiteA, topSiteB, topSiteC], {
      columnsPerPage: 1,
      rowsPerPage: 1,
      canAddSite: false,
      sponsoredSites: [],
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
