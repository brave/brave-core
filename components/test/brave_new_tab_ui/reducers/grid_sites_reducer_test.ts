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

  describe('GRID_SITES_DATA_UPDATED', () => {
    let tilesUpdatedStub: jest.SpyInstance

    beforeEach(() => {
      tilesUpdatedStub = jest
        .spyOn(gridSitesState, 'tilesUpdated')
    })
    afterEach(() => {
      tilesUpdatedStub.mockRestore()
    })

    it('calls tilesUpdated with the correct args', () => {
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_DATA_UPDATED,
        payload: { gridSites }
      })

      expect(tilesUpdatedStub).toBeCalledTimes(1)
      expect(tilesUpdatedStub)
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
  describe('GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION', () => {
    let showTilesRemovedNoticeStub: jest.SpyInstance

    beforeEach(() => {
      showTilesRemovedNoticeStub = jest
        .spyOn(gridSitesState, 'showTilesRemovedNotice')
    })
    afterEach(() => {
      showTilesRemovedNoticeStub.mockRestore()
    })

    it('calls showTilesRemovedNotice with the correct args', () => {
      const shouldShow: boolean = true
      gridSitesReducer(undefined, {
        type: types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION,
        payload: { shouldShow }
      })

      expect(showTilesRemovedNoticeStub).toBeCalledTimes(1)
      expect(showTilesRemovedNoticeStub)
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
