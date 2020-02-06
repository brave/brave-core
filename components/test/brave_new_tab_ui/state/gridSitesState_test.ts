// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// State helpers
import * as gridSitesState from '../../../brave_new_tab_ui/state/gridSitesState'

// Helpers
import { generateGridSiteProperties } from '../../../brave_new_tab_ui/helpers/newTabUtils'
import { defaultState } from '../../../brave_new_tab_ui/storage'

const newTopSite1: chrome.topSites.MostVisitedURL = {
  url: 'https://brave.com',
  title: 'brave!'
}

const newTopSite2: chrome.topSites.MostVisitedURL = {
  url: 'https://clifton.io',
  title: 'BSC]]'
}

const gridSites: NewTab.Site[] = [{
  ...newTopSite1,
  ...generateGridSiteProperties(0, newTopSite1)
}, {
  ...newTopSite2,
  ...generateGridSiteProperties(1, newTopSite2)
}]

describe('gridSitesState', () => {
  describe('gridSitesReducerSetFirstRenderData', () => {
    it('does not populate state.gridSites list if url already exist within the list', () => {
      const newState: NewTab.State = {
        ...defaultState,
        gridSites: [generateGridSiteProperties(0, newTopSite1)]
      }
      const assertion = gridSitesState
        .gridSitesReducerSetFirstRenderData(newState, [
          newTopSite1,
          newTopSite1,
          newTopSite1,
          newTopSite1
        ])

      expect(assertion.gridSites).toHaveLength(1)
    })
    it('populate state.gridSites list if urls are different', () => {
      const assertion = gridSitesState
        .gridSitesReducerSetFirstRenderData(defaultState, [
          newTopSite1,
          newTopSite2
        ])

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('gridSitesReducerDataUpdated', () => {
    it('update state.gridSites list', () => {
      const assertion = gridSitesState
        .gridSitesReducerDataUpdated(defaultState, gridSites)

      expect(assertion.gridSites).toHaveLength(2)
    })
    it('preserve own pinnedIndex position after a new site is added', () => {
      const pinnedIndex: number = 1
      const pinnedSite: NewTab.Site = {
        ...gridSites[0],
        url: 'https://cezaraugusto.net',
        title: `pinned position ${pinnedIndex}`,
        pinnedIndex
      }
      const newGridSites: NewTab.Sites[] = [
        {
          ...gridSites[0],
          url: 'https://brave.com',
          title: 'not pinned position 0'
        },
        pinnedSite,
        {
          ...gridSites[0],
          url: 'https://clifton.io',
          title: 'not pinned position 2'
        }
      ]

      const newState: NewTab.State = {
        ...defaultState,
        gridSites: newGridSites
      }
      // add a new site on top of gridSites after a tile
      // have been pinned
      const newestGridSites: NewTab.Site[] = [
        ...newGridSites,
        {
          ...gridSites[0],
          url: 'https://petemill.com',
          title: 'not pinned TBD position 0'
        }
      ]
      const assertion = gridSitesState
        .gridSitesReducerDataUpdated(newState, newestGridSites)

      expect(assertion.gridSites).toHaveLength(4)
      // pinned tiles cannot be moved
      expect(assertion.gridSites[pinnedIndex]).toEqual(pinnedSite)
    })
    it('preserve pinnedIndex positions after random reordering', () => {
      // just an utility for our test
      // adapted from https://stackoverflow.com/a/6274381/4902448
      const shuffle = (arr: Array<any>) => {
        for (let i = arr.length - 1; i > 0; i--) {
          const j = Math.floor(Math.random() * (i + 1))
          ;[arr[i], arr[j]] = [arr[j], arr[i]]
        }
        return arr
      }

      const pinnedIndex: number = 1
      const pinnedSite: NewTab.Site = {
        ...gridSites[0],
        url: 'https://cezaraugusto.net',
        title: `pinned position ${pinnedIndex}`,
        pinnedIndex
      }
      const newGridSites: NewTab.Sites[] = [
        {
          ...gridSites[0],
          url: 'https://brave.com',
          title: 'not pinned position 0'
        },
        pinnedSite,
        {
          ...gridSites[0],
          url: 'https://clifton.io',
          title: 'not pinned position 2'
        }
      ]

      const newState: NewTab.State = {
        ...defaultState,
        gridSites: newGridSites
      }

      const assertion = gridSitesState
        .gridSitesReducerDataUpdated(newState, shuffle(newGridSites))

      // pinned tiles should preserve position after shuffle
      expect(assertion.gridSites[pinnedIndex]).toEqual(pinnedSite)
    })
  })
  describe('gridSitesReducerToggleSitePinned', () => {
    it('set own pinnedIndex value if property is undefined', () => {
      const expectedIndex: number = 1
      const newState: NewTab.State = { ...defaultState, gridSites }

      const assertion = gridSitesState
        .gridSitesReducerToggleSitePinned(newState, gridSites[expectedIndex])

      expect(assertion.gridSites[expectedIndex])
        .toHaveProperty('pinnedIndex', expectedIndex)
    })
    it('set own pinnedIndex value to undefined if property is defined', () => {
      const pinnedSite: NewTab.Site = { ...gridSites[1], pinnedIndex: 1337 }
      const newState: NewTab.State = { ...defaultState, gridSites: [pinnedSite] }

      const assertion = gridSitesState
        .gridSitesReducerToggleSitePinned(newState, pinnedSite)

      expect(assertion.gridSites[0])
        .toHaveProperty('pinnedIndex', undefined)
    })
    it('does not add length to the list after a site is pinned', () => {
      const pinnedSite: NewTab.Site = {
        ...gridSites[1],
        title: 'some site',
        url: 'fake.com',
        pinnedIndex: undefined
      }
      const newState: NewTab.State = { ...defaultState, gridSites: [ ...gridSites, pinnedSite ] }

      expect(newState.gridSites).toHaveLength(3)

      const assertion = gridSitesState
        .gridSitesReducerToggleSitePinned(newState, pinnedSite)

      expect(assertion.gridSites).toHaveLength(3)
    })
  })
  describe('gridSitesReducerRemoveSite', () => {
    it('remove a site from state.gridSites list', () => {
      const removedSite: NewTab.Site = gridSites[1]
      const newState: NewTab.State = { ...defaultState, gridSites }

      const assertion = gridSitesState
        .gridSitesReducerRemoveSite(newState, removedSite)

      expect(assertion.gridSites).toHaveLength(1)
    })
  })
  describe('gridSitesReducerUndoRemoveSite', () => {
    it('push an item from the state.removedSites list back to state.gridSites list', () => {
      const removedSite: NewTab.Site = { ...gridSites[1], url: 'https://example.com' }
      const newState: NewTab.State = {
        ...defaultState,
        gridSites,
        removedSites: [removedSite]
      }

      const assertion = gridSitesState
        .gridSitesReducerUndoRemoveSite(newState)

      expect(assertion.gridSites).toHaveLength(3)
    })
    it('do not push an item from state.gridSites if url exists inside the list', () => {
      const removedSite: NewTab.Site = { ...gridSites[1] }
      const newState: NewTab.State = {
        ...defaultState,
        gridSites,
        removedSites: [removedSite]
      }

      const assertion = gridSitesState
        .gridSitesReducerUndoRemoveSite(newState)

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('gridSitesReducerUndoRemoveAllSites', () => {
    it('push all items from state.removedSites list back to state.gridSites list', () => {
      const removedSites: NewTab.Site[] = [{
        ...gridSites[0],
        url: 'https://example.com'
      }, {
        ...gridSites[1],
        url: 'https://another-example.com'
      }]

      const newState: NewTab.State = {
        ...defaultState,
        gridSites,
        removedSites: removedSites
      }

      const assertion = gridSitesState
        .gridSitesReducerUndoRemoveAllSites(newState)

      expect(assertion.gridSites).toHaveLength(4)
    })
    it('do not push any item to state.gridSites if url exists inside the list', () => {
      const sites: NewTab.Sites[] = gridSites
      const newState: NewTab.State = {
        ...defaultState,
        gridSites: sites,
        removedSites: sites
      }

      const assertion = gridSitesState
        .gridSitesReducerUndoRemoveAllSites(newState)

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('gridSitesReducerUpdateSiteBookmarkInfo', () => {
    it('update own bookmarkInfo with the specified value', () => {
      const topSiteUrl: NewTab.Site = gridSites[0].url
      const sites: NewTab.Sites[] = [{ ...gridSites[0], bookmarkInfo: 'NEW_INFO' }]
      const newState: NewTab.State = { ...defaultState, gridSites: sites }

      const assertion = gridSitesState
        .gridSitesReducerUpdateSiteBookmarkInfo(newState, topSiteUrl)

      expect(assertion.gridSites[0])
        .toHaveProperty('bookmarkInfo', 'NEW_INFO')
    })
  })
  describe('gridSitesReducerToggleTopSiteBookmarked', () => {
    it('add own add bookmarkInfo if url has no data', () => {
      const siteUrl: string = gridSites[0].url
      const newState: NewTab.State = { ...defaultState, gridSites }

      const assertion = gridSitesState
        .gridSitesReducerToggleSiteBookmarkInfo(newState, siteUrl, undefined)

      expect(assertion.gridSites[0].bookmarkInfo).not.toBeUndefined()
    })
    it('remove own bookmarkInfo if url has data', () => {
      const siteUrl: string = gridSites[0].url
      const topSiteBookmarkInfo: chrome.bookmarks.BookmarkTreeNode = {
        title: 'cool bookmark',
        id: ''
      }
      const newState: NewTab.State = { ...defaultState, gridSites }

      const assertion = gridSitesState
        .gridSitesReducerToggleSiteBookmarkInfo(newState, siteUrl, topSiteBookmarkInfo)

      expect(assertion.gridSites[0].bookmarkInfo).toBeUndefined()
    })
  })
  describe('gridSitesReducerAddSiteOrSites', () => {
    it('add sites to state.gridSites list', () => {
      const newSite: NewTab.Site = { ...gridSites[0], url: 'https://example.com' }
      const newState: NewTab.State = { ...defaultState, gridSites }

      const assertion = gridSitesState
        .gridSitesReducerAddSiteOrSites(newState, newSite)

      expect(assertion.gridSites).toHaveLength(3)
    })
  })
  describe('gridSitesReducerShowSiteRemovedNotification', () => {
    it('update state with the specified payload value', () => {
      const shouldShow: boolean = true

      const assertion = gridSitesState
        .gridSitesReducerShowSiteRemovedNotification(defaultState, shouldShow)

      expect(assertion.shouldShowSiteRemovedNotification).toBe(true)
    })
  })
})
