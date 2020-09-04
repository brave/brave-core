// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// State helpers
import * as gridSitesState from '../../../brave_new_tab_ui/state/gridSitesState'

// Helpers
import * as storage from '../../../brave_new_tab_ui/storage/grid_sites_storage'

const newTopSite1: chrome.topSites.MostVisitedURL = {
  url: 'https://brave.com',
  title: 'brave!'
}

const newTopSite2: chrome.topSites.MostVisitedURL = {
  url: 'https://clifton.io',
  title: 'BSC]]'
}

let top_site_id = 1
const generateGridSiteProperties = (
  index: number,
  topSite: chrome.topSites.MostVisitedURL,
  fromLegacyData?: boolean
): NewTab.Site => {
  return {
    title: topSite.title,
    url: topSite.url,
    id: top_site_id++,
    // In the legacy version of topSites the pinnedIndex
    // was the site index itself.
    pinnedIndex: fromLegacyData ? index : undefined,
    defaultSRTopSite: false
  }
}

const gridSites: NewTab.Site[] = [{
  ...newTopSite1,
  ...generateGridSiteProperties(0, newTopSite1)
}, {
  ...newTopSite2,
  ...generateGridSiteProperties(1, newTopSite2)
}]

describe('gridSitesState', () => {
  describe('tilesUpdated', () => {
    it('update state.gridSites list', () => {
      const assertion = gridSitesState
        .tilesUpdated(storage.initialGridSitesState, gridSites)

      expect(assertion.gridSites).toHaveLength(2)
    })
  })
  describe('showTilesRemovedNotice', () => {
    it('update state with the specified payload value', () => {
      const shouldShow: boolean = true

      const assertion = gridSitesState
        .showTilesRemovedNotice(storage.initialGridSitesState, shouldShow)

      expect(assertion.shouldShowSiteRemovedNotification).toBe(true)
    })
  })
})
