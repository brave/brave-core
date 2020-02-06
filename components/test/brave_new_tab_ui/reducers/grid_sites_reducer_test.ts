// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import gridSitesReducer from '../../../brave_new_tab_ui/reducers/grid_sites_reducer'
import * as storage from '../../../brave_new_tab_ui/storage'
import { types } from '../../../brave_new_tab_ui/constants/grid_sites_types'
import * as gridSitesState from '../../../brave_new_tab_ui/state/gridSitesState'

const bookmarkInfo: chrome.bookmarks.BookmarkTreeNode = {
  dateAdded: 123123,
  id: '',
  index: 1337,
  parentId: '',
  title: 'brave',
  url: 'https://brave.com'
}
const topSites: chrome.topSites.MostVisitedURL[] = [{
  url: 'https://brave.com',
  title: 'brave'
}, {
  url: 'https://cezaraugusto.net',
  title: 'cezar augusto'
}]
const gridSites: NewTab.Site[] = [{
  ...topSites[0],
  id: 'topsite-0',
  favicon: '',
  letter: 'b',
  pinnedIndex: undefined,
  bookmarkInfo
}, {
  ...topSites[1],
  id: 'topsite-1',
  favicon: '',
  letter: 'c',
  pinnedIndex: undefined,
  bookmarkInfo: undefined
}]

describe('gridSitesReducer', () => {
  describe('Handle initial state', () => {
    it('returns the initial state when state is undefined', () => {
      const assertion = gridSitesReducer(
        undefined,
        { type: undefined, payload: undefined }
      )

      expect(assertion).toEqual(storage.defaultState)
    })
  })

  describe('GRID_SITES_SET_FIRST_RENDER_DATA', () => {
    let gridSitesReducerSetFirstRenderDataStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerSetFirstRenderDataStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerSetFirstRenderData')
    })
    afterEach(() => {
      gridSitesReducerSetFirstRenderDataStub.mockRestore()
    })

    it('calls gridSitesReducerSetFirstRenderData with the correct args', () => {
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites: [] }
      })

      expect(gridSitesReducerSetFirstRenderDataStub).toBeCalledTimes(1)
      expect(gridSitesReducerSetFirstRenderDataStub)
        .toBeCalledWith(storage.defaultState, [])
    })
    it('populate state.gridSites list with Chromium topSites data', () => {
      const assertion = gridSitesReducer(storage.defaultState, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites }
      })

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('GRID_SITES_DATA_UPDATED', () => {
    let gridSitesReducerDataUpdatedStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerDataUpdatedStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerDataUpdated')
    })
    afterEach(() => {
      gridSitesReducerDataUpdatedStub.mockRestore()
    })

    it('calls gridSitesReducerDataUpdated with the correct args', () => {
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_DATA_UPDATED,
        payload: { gridSites }
      })

      expect(gridSitesReducerDataUpdatedStub).toBeCalledTimes(1)
      expect(gridSitesReducerDataUpdatedStub)
        .toBeCalledWith(storage.defaultState, gridSites)
    })
    it('update state.gridSites list', () => {
      const assertion = gridSitesReducer(storage.defaultState, {
        type: types.GRID_SITES_DATA_UPDATED,
        payload: { gridSites }
      })

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('GRID_SITES_TOGGLE_SITE_PINNED', () => {
    let gridSitesReducerToggleSitePinnedStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerToggleSitePinnedStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerToggleSitePinned')
    })
    afterEach(() => {
      gridSitesReducerToggleSitePinnedStub.mockRestore()
    })

    it('calls gridSitesReducerToggleSitePinned with the correct args', () => {
      const site: NewTab.Site = gridSites[0]
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_TOGGLE_SITE_PINNED,
        payload: { pinnedSite: site }
      })

      expect(gridSitesReducerToggleSitePinnedStub).toBeCalledTimes(1)
      expect(gridSitesReducerToggleSitePinnedStub)
        .toBeCalledWith(storage.defaultState, site)
    })
    it('set own pinnedIndex value if property is undefined', () => {
      const pinnedSite: NewTab.Site = { ...gridSites[1], pinnedIndex: 1337 }
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites: [pinnedSite]
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_TOGGLE_SITE_PINNED,
        payload: { pinnedSite }
      })

      expect(assertion.gridSites[0])
        .toHaveProperty('pinnedIndex', undefined)
    })
    it('set own pinnedIndex value to undefined if property is defined', () => {
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_TOGGLE_SITE_PINNED,
        payload: { pinnedSite: gridSites[1] }
      })

      expect(assertion.gridSites[1])
        .toHaveProperty('pinnedIndex', 1)
    })
  })
  describe('GRID_SITES_REMOVE_SITE', () => {
    let gridSitesReducerRemoveSiteStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerRemoveSiteStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerRemoveSite')
    })
    afterEach(() => {
      gridSitesReducerRemoveSiteStub.mockRestore()
    })

    it('calls gridSitesReducerRemoveSite with the correct args', () => {
      const site: NewTab.Site = gridSites[0]
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_REMOVE_SITE,
        payload: { removedSite: site }
      })

      expect(gridSitesReducerRemoveSiteStub).toBeCalledTimes(1)
      expect(gridSitesReducerRemoveSiteStub)
        .toBeCalledWith(storage.defaultState, site)
    })
    it('remove a site from state.gridSites list', () => {
      const removedSite: NewTab.Site = gridSites[1]
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_REMOVE_SITE,
        payload: { removedSite: removedSite }
      })

      expect(assertion.gridSites).toHaveLength(1)
    })
  })
  describe('GRID_SITES_UNDO_REMOVE_SITE', () => {
    let gridSitesReducerUndoRemoveSiteStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerUndoRemoveSiteStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerUndoRemoveSite')
    })
    afterEach(() => {
      gridSitesReducerUndoRemoveSiteStub.mockRestore()
    })

    it('calls gridSitesReducerUndoRemoveSite with the correct args', () => {
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_UNDO_REMOVE_SITE,
        payload: undefined
      })

      expect(gridSitesReducerUndoRemoveSiteStub).toBeCalledTimes(1)
      expect(gridSitesReducerUndoRemoveSiteStub)
        .toBeCalledWith(storage.defaultState)
    })
    it('push an item from state.removedSites list back to state.gridSites list', () => {
      const removedSite: NewTab.Site = {
        ...gridSites[1],
        url: 'https://example.com'
      }
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites,
        removedSites: [removedSite]
      }
      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_UNDO_REMOVE_SITE,
        payload: undefined
      })

      expect(assertion.gridSites).toHaveLength(3)
    })
    it('do not push an item from state.gridSites if url exists inside the list', () => {
      const removedSite: NewTab.Site = { ...gridSites[1] }
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites,
        removedSites: [removedSite]
      }
      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_UNDO_REMOVE_SITE,
        payload: undefined
      })

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('GRID_SITES_UNDO_REMOVE_ALL_SITES', () => {
    let gridSitesReducerUndoRemoveAllSitesStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerUndoRemoveAllSitesStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerUndoRemoveAllSites')
    })
    afterEach(() => {
      gridSitesReducerUndoRemoveAllSitesStub.mockRestore()
    })

    it('calls gridSitesReducerUndoRemoveAllSites with the correct args', () => {
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_UNDO_REMOVE_ALL_SITES,
        payload: undefined
      })

      expect(gridSitesReducerUndoRemoveAllSitesStub).toBeCalledTimes(1)
      expect(gridSitesReducerUndoRemoveAllSitesStub)
        .toBeCalledWith(storage.defaultState)
    })
    it('push all items from state.removedSites list back to state.gridSites list', () => {
      const removedSites: NewTab.Site[] = [{
        ...gridSites[0],
        url: 'https://example.com'
      }, {
        ...gridSites[1],
        url: 'https://another-example.com'
      }]
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites,
        removedSites: removedSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_UNDO_REMOVE_ALL_SITES,
        payload: undefined
      })

      expect(assertion.gridSites).toHaveLength(4)
    })
    it('do not push any item to state.gridSites if url exists inside the list', () => {
      const sites: NewTab.Sites[] = gridSites
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites: sites,
        removedSites: sites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_UNDO_REMOVE_ALL_SITES,
        payload: undefined
      })

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('GRID_SITES_UPDATE_SITE_BOOKMARK_INFO', () => {
    let gridSitesReducerUpdateSiteBookmarkInfoStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerUpdateSiteBookmarkInfoStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerUpdateSiteBookmarkInfo')
    })
    afterEach(() => {
      gridSitesReducerUpdateSiteBookmarkInfoStub.mockRestore()
    })

    it('calls gridSitesReducerUpdateSiteBookmarkInfo with the correct args', () => {
      const topSiteBookmarkInfo: NewTab.Site = gridSites[0].bookmarkInfo
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_UPDATE_SITE_BOOKMARK_INFO,
        payload: { bookmarkInfo: topSiteBookmarkInfo }
      })

      expect(gridSitesReducerUpdateSiteBookmarkInfoStub).toBeCalledTimes(1)
      expect(gridSitesReducerUpdateSiteBookmarkInfoStub)
        .toBeCalledWith(storage.defaultState, topSiteBookmarkInfo)
    })
    it('update own bookmarkInfo with the specified value', () => {
      const topSiteBookmarkInfo: NewTab.Site = gridSites[0].bookmarkInfo
      const sites: NewTab.Sites[] = [{
        ...gridSites[0], bookmarkInfo: 'NEW_INFO'
      }]
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites: sites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_UPDATE_SITE_BOOKMARK_INFO,
        payload: { bookmarkInfo: topSiteBookmarkInfo }
      })

      expect(assertion.gridSites[0])
        .toHaveProperty('bookmarkInfo', 'NEW_INFO')
    })
  })
  describe('GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO', () => {
    let gridSitesReducerToggleSiteBookmarkInfoStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerToggleSiteBookmarkInfoStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerToggleSiteBookmarkInfo')
    })
    afterEach(() => {
      gridSitesReducerToggleSiteBookmarkInfoStub.mockRestore()
    })

    it('calls gridSitesReducerToggleSiteBookmarkInfo with the correct args', () => {
      const siteUrl: string = gridSites[0].url
      const topSiteBookmarkInfo: chrome.bookmarks.BookmarkTreeNode
        = bookmarkInfo
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO,
        payload: {
          url: siteUrl,
          bookmarkInfo: topSiteBookmarkInfo
        }
      })

      expect(gridSitesReducerToggleSiteBookmarkInfoStub).toBeCalledTimes(1)
      expect(gridSitesReducerToggleSiteBookmarkInfoStub)
        .toBeCalledWith(storage.defaultState, siteUrl, topSiteBookmarkInfo)
    })
    it('add own add bookmarkInfo if url has no data', () => {
      const siteUrl: string = gridSites[0].url
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO,
        payload: {
          url: siteUrl,
          bookmarkInfo: undefined
        }
      })

      expect(assertion.gridSites[0].bookmarkInfo).not.toBeUndefined()
    })
    it('remove own bookmarkInfo if url has data', () => {
      const siteUrl: string = gridSites[0].url
      const topSiteBookmarkInfo: NewTab.Site = gridSites[0].bookmarkInfo
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO,
        payload: {
          url: siteUrl,
          bookmarkInfo: topSiteBookmarkInfo
        }
      })

      expect(assertion.gridSites[0].bookmarkInfo).toBeUndefined()
    })
  })
  describe('GRID_SITES_ADD_SITES', () => {
    let gridSitesReducerAddSiteOrSitesStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerAddSiteOrSitesStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerAddSiteOrSites')
    })
    afterEach(() => {
      gridSitesReducerAddSiteOrSitesStub.mockRestore()
    })

    it('calls gridSitesReducerAddSiteOrSites with the correct args', () => {
      const site: NewTab.Site = gridSites[0]
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_ADD_SITES,
        payload: { site: site }
      })

      expect(gridSitesReducerAddSiteOrSitesStub).toBeCalledTimes(1)
      expect(gridSitesReducerAddSiteOrSitesStub)
        .toBeCalledWith(storage.defaultState, site)
    })
    it('add sites to state.gridSites list', () => {
      const newSite: NewTab.Site = {
        ...gridSites[0],
        url: 'https://example.com'
      }
      const newStateWithGridSites: NewTab.State = {
        ...storage.defaultState,
        gridSites
      }

      const assertion = gridSitesReducer(newStateWithGridSites, {
        type: types.GRID_SITES_ADD_SITES,
        payload: { site: newSite }
      })

      expect(assertion.gridSites).toHaveLength(3)
    })
  })
  describe('GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION', () => {
    let gridSitesReducerShowSiteRemovedNotificationStub: jest.SpyInstance

    beforeEach(() => {
      gridSitesReducerShowSiteRemovedNotificationStub = jest
        .spyOn(gridSitesState, 'gridSitesReducerShowSiteRemovedNotification')
    })
    afterEach(() => {
      gridSitesReducerShowSiteRemovedNotificationStub.mockRestore()
    })

    it('calls gridSitesReducerShowSiteRemovedNotification with the correct args', () => {
      const shouldShow: boolean = true
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION,
        payload: { shouldShow }
      })

      expect(gridSitesReducerShowSiteRemovedNotificationStub).toBeCalledTimes(1)
      expect(gridSitesReducerShowSiteRemovedNotificationStub)
        .toBeCalledWith(storage.defaultState, shouldShow)
    })
    it('update state with the specified payload value', () => {
      const assertion = gridSitesReducer(storage.defaultState, {
        type: types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION,
        payload: {
          shouldShow: true
        }
      })

      expect(assertion.shouldShowSiteRemovedNotification).toBe(true)
    })
  })
})
