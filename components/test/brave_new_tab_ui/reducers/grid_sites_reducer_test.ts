// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import gridSitesReducer from '../../../brave_new_tab_ui/reducers/grid_sites_reducer'
import * as storage from '../../../brave_new_tab_ui/storage/grid_sites_storage'
import { types } from '../../../brave_new_tab_ui/constants/grid_sites_types'
import * as gridSitesState from '../../../brave_new_tab_ui/state/gridSitesState'

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
  pinnedIndex: undefined
}, {
  ...topSites[1],
  id: 'topsite-1',
  favicon: '',
  letter: 'c',
  pinnedIndex: undefined
}]

describe('gridSitesReducer', () => {
  describe('Handle initial state', () => {
    it('returns the initial state when state is undefined', () => {
      const assertion = gridSitesReducer(
        undefined,
        { type: undefined, payload: undefined }
      )

      expect(assertion).toEqual(storage.initialGridSitesState)
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
        .toBeCalledWith(storage.initialGridSitesState, [])
    })
    it('populate state.gridSites list with Chromium topSites data', () => {
      const assertion = gridSitesReducer(storage.initialGridSitesState, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites }
      })

      expect(assertion.gridSites).toHaveLength(2)
    })
    it('check defaultSRTopSite is set properly if gridSites does not have that propeerty', () => {
      let state: NewTab.GridSitesState = storage.initialGridSitesState
      state = {
        ...state,
        gridSites: [
          {
            url: 'www.google.com',
            title: 'Google',
            id: 'topsite-2',
            favicon: '',
            letter: 'c',
            pinnedIndex: 0
          },
          {
            url: 'www.brave.com',
            title: 'Brave Software',
            id: 'topsite-1',
            favicon: '',
            letter: 'c',
            pinnedIndex: undefined
          }
        ]
      }

      const topSites = [
        {
          url: 'www.brave.com',
          title: 'brave'
        }, {
          url: 'www.cnn.com',
          title: 'cnn'
        }
      ]

      const defaultSuperReferralTopSites = [
        {
          url: 'www.google.com',
          title: 'Google',
          favicon: '',
          pinnedIndex: 0
        }
      ]

      const assertion = gridSitesReducer(state, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites, defaultSuperReferralTopSites }
      })

      expect(assertion.gridSites[0].defaultSRTopSite).toBe(true)
      expect(assertion.gridSites[1].defaultSRTopSite).toBe(false)
    })

    it('check gridSites does not have sites that are removed from topSites', () => {
      let state: NewTab.GridSitesState = storage.initialGridSitesState
      state = {
        ...state,
        gridSites: [
          {
            url: 'www.basicattentiontoken.org',
            title: 'BAT',
            id: 'topsite-0',
            favicon: '',
            letter: 'b',
            pinnedIndex: 0,
            defaultSRTopSite: false
          },
          {
            url: 'www.brave.com',
            title: 'Brave Software',
            id: 'topsite-1',
            favicon: '',
            letter: 'c',
            pinnedIndex: undefined,
            defaultSRTopSite: false
          },
          {
            url: 'www.google.com',
            title: 'Google',
            id: 'topsite-2',
            favicon: '',
            letter: 'c',
            pinnedIndex: undefined,
            defaultSRTopSite: false
          }
        ]
      }

      const topSites = [
        {
          url: 'www.brave.com',
          title: 'brave'
        }, {
          url: 'www.cnn.com',
          title: 'cnn'
        }
      ]
      const assertion = gridSitesReducer(state, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites }
      })

      // Initial state has google site but it's deleted because topSites doesn't
      // have it.
      expect(assertion.gridSites).toHaveLength(2)
      expect(assertion.gridSites[0].url).toBe('www.cnn.com')
      expect(assertion.gridSites[1].url).toBe('www.brave.com')
    })
    it('check sites from SR are not deleted from gridSites', () => {
      let state: NewTab.GridSitesState = storage.initialGridSitesState
      state = {
        ...state,
        gridSites: [
          {
            url: 'www.basicattentiontoken.org',
            title: 'BAT',
            id: 'topsite-0',
            favicon: '',
            letter: 'b',
            pinnedIndex: 0,
            defaultSRTopSite: true
          },
          {
            url: 'www.brave.com',
            title: 'Brave Software',
            id: 'topsite-1',
            favicon: '',
            letter: 'c',
            pinnedIndex: undefined,
            defaultSRTopSite: false
          },
          {
            url: 'www.google.com',
            title: 'Google',
            id: 'topsite-2',
            favicon: '',
            letter: 'c',
            pinnedIndex: undefined,
            defaultSRTopSite: false
          }
        ]
      }

      const topSites = [
        {
          url: 'www.brave.com',
          title: 'brave'
        }, {
          url: 'www.cnn.com',
          title: 'cnn'
        }
      ]

      const assertion = gridSitesReducer(state, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites }
      })

      // topSites doesn't have bat site but it's in SR's default top sites.
      // So, it's not deleted from gritSites.
      expect(assertion.gridSites).toHaveLength(3)
      expect(assertion.gridSites[0].url).toBe('www.basicattentiontoken.org')
      expect(assertion.gridSites[1].url).toBe('www.cnn.com')
      expect(assertion.gridSites[2].url).toBe('www.brave.com')
    })
    it('populate state.gridSites list without duplicates', () => {
      const brandNewSite: NewTab.Site = {
        id: 'topsite-000',
        url: 'https://.com',
        title: 'g. clooney free propaganda',
        pinnedIndex: 0,
        favicon: '',
        letter: ''
      }
      const veryRepetitiveSite: NewTab.Site = {
        id: 'topsite-111',
        url: 'https://serg-loves-pokemon-4ever.com',
        title: 'pokemon fan page',
        pinnedIndex: 0,
        favicon: '',
        letter: ''
      }

      const veryRepetitiveSiteList: NewTab.Site[] = [
        veryRepetitiveSite,
        veryRepetitiveSite,
        veryRepetitiveSite,
        veryRepetitiveSite,
        veryRepetitiveSite
      ]

      // Add repetitiveSiteList everywhere. Dupe party.
      const veryRepetitiveInitialGridSitesState: NewTab.GridSitesState = {
        gridSites: veryRepetitiveSiteList,
        removedSites: veryRepetitiveSiteList,
        shouldShowSiteRemovedNotification: false,
        legacy: {
          pinnedTopSites: veryRepetitiveSiteList,
          ignoredTopSites: veryRepetitiveSiteList
        }
      }

      const assertion = gridSitesReducer(veryRepetitiveInitialGridSitesState, {
        type: types.GRID_SITES_SET_FIRST_RENDER_DATA,
        payload: { topSites: [ brandNewSite, veryRepetitiveSite ] }
      })
      // We should see just one repetitive site and the new
      // one added on the payload above
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
        .toBeCalledWith(storage.initialGridSitesState, gridSites)
    })
    it('update state.gridSites list', () => {
      const assertion = gridSitesReducer(storage.initialGridSitesState, {
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
        .toBeCalledWith(storage.initialGridSitesState, site)
    })
    it('set own pinnedIndex value if property is undefined', () => {
      const pinnedSite: NewTab.Site = { ...gridSites[1], pinnedIndex: 1337 }
      const newStateWithGridSites: NewTab.State = {
        ...storage.initialGridSitesState,
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
        ...storage.initialGridSitesState,
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
        .toBeCalledWith(storage.initialGridSitesState, site)
    })
    it('remove a site from state.gridSites list', () => {
      const removedSite: NewTab.Site = gridSites[1]
      const newStateWithGridSites: NewTab.State = {
        ...storage.initialGridSitesState,
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
        .toBeCalledWith(storage.initialGridSitesState)
    })
    it('push an item from state.removedSites list back to state.gridSites list', () => {
      const removedSite: NewTab.Site = {
        ...gridSites[1],
        url: 'https://example.com',
        id: 'topsite-999999'
      }
      const newStateWithGridSites: NewTab.State = {
        ...storage.initialGridSitesState,
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
        ...storage.initialGridSitesState,
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
        .toBeCalledWith(storage.initialGridSitesState)
    })
    it('push all items from state.removedSites list back to state.gridSites list', () => {
      const removedSites: NewTab.Site[] = [{
        ...gridSites[0],
        url: 'https://example.com',
        id: 'topsite-999999'
      }, {
        ...gridSites[1],
        url: 'https://another-example.com',
        id: 'topsite-999998'
      }]
      const newStateWithGridSites: NewTab.State = {
        ...storage.initialGridSitesState,
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
        ...storage.initialGridSitesState,
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
        .toBeCalledWith(storage.initialGridSitesState, site)
    })
    it('add sites to state.gridSites list', () => {
      const newSite: NewTab.Site = {
        ...gridSites[0],
        url: 'https://example.com',
        id: 'topsite-99999'
      }
      const newStateWithGridSites: NewTab.State = {
        ...storage.initialGridSitesState,
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
        .toBeCalledWith(storage.initialGridSitesState, shouldShow)
    })
    it('update state with the specified payload value', () => {
      const assertion = gridSitesReducer(storage.initialGridSitesState, {
        type: types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION,
        payload: {
          shouldShow: true
        }
      })

      expect(assertion.shouldShowSiteRemovedNotification).toBe(true)
    })
  })
})
