/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { renderHook } from '@testing-library/react'
import { createStateStore } from '$web-common/state_store'
import { TopSitesContext } from '../../context/top_sites_context'
import {
  SponsoredSite,
  TopSite,
  TopSitesListKind,
  TopSitesState,
} from '../../state/top_sites_store'
import {
  topSitesWithoutSponsoredSiteDuplicates,
  useTopSitesGridItems,
} from './top_sites_grid_items'

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

describe('useTopSitesGridItems', () => {
  it('should return an empty list when there are no top sites and no sponsored sites', () => {
    const store = createStore({ topSites: [], sponsoredSites: [] })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: false }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([])
  })

  it('should place sponsored sites ahead of top sites, each carrying its original index', () => {
    const topSiteA = createTopSite('https://foo.com')
    const topSiteB = createTopSite('https://qux.com')
    const sponsoredSite = createSponsoredSite('https://baz.com')
    const store = createStore({
      topSites: [topSiteA, topSiteB],
      sponsoredSites: [sponsoredSite],
    })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: false }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([
      { type: 'sponsored-site', site: sponsoredSite },
      { type: 'top-site', site: topSiteA, index: 0 },
      { type: 'top-site', site: topSiteB, index: 1 },
    ])
  })

  it('should omit a top site deduplicated against a sponsored site, preserving remaining indices', () => {
    const duplicateTopSite = createTopSite('https://bar.com')
    const remainingTopSite = createTopSite('https://qux.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const store = createStore({
      topSites: [duplicateTopSite, remainingTopSite],
      sponsoredSites: [sponsoredSite],
    })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: false }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([
      { type: 'sponsored-site', site: sponsoredSite },
      { type: 'top-site', site: remainingTopSite, index: 1 },
    ])
  })

  it('should preserve original indices for sites surrounding a middle duplicate', () => {
    const topSiteA = createTopSite('https://foo.com')
    const duplicateTopSite = createTopSite('https://bar.com')
    const topSiteC = createTopSite('https://qux.com')
    const sponsoredSite = createSponsoredSite('https://bar.com')
    const store = createStore({
      topSites: [topSiteA, duplicateTopSite, topSiteC],
      sponsoredSites: [sponsoredSite],
    })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: false }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([
      { type: 'sponsored-site', site: sponsoredSite },
      { type: 'top-site', site: topSiteA, index: 0 },
      { type: 'top-site', site: topSiteC, index: 2 },
    ])
  })

  it('should return only an add-button when there are no sites at all', () => {
    const store = createStore({ topSites: [], sponsoredSites: [] })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: true }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([{ type: 'add-button' }])
  })

  it('should hide sponsored sites when the user has turned them off', () => {
    const topSite = createTopSite('https://bar.com')
    const sponsoredSites = [createSponsoredSite('https://bar.com')]
    const store = createStore({
      topSites: [topSite],
      sponsoredSites,
      showSponsoredSites: false,
    })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: false }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([
      { type: 'top-site', site: topSite, index: 0 },
    ])
  })

  it('should append an add-button when adding a site is allowed', () => {
    const topSite = createTopSite('https://foo.com')
    const store = createStore({ topSites: [topSite] })

    const { result } = renderHook(
      () => useTopSitesGridItems({ canAddSite: true }),
      { wrapper: createWrapper(store) },
    )

    expect(result.current).toEqual([
      { type: 'top-site', site: topSite, index: 0 },
      { type: 'add-button' },
    ])
  })
})
