// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Utils
import { debounce } from '../../common/debounce'
const oldkeyName = 'grid-sites-data-v1'
const newkeyName = 'grid-sites-data-v2'

export const initialGridSitesState: NewTab.GridSitesState = {
  gridSites: [],
  removedSites: [],
  shouldShowSiteRemovedNotification: false
}

export const load = (): NewTab.GridSitesState => {
  // Cleanup legacy localStorage (not needed anymore)
  if (window.localStorage.getItem(oldkeyName)) {
    window.localStorage.removeItem(oldkeyName)
  }
  const data: string | null = window.sessionStorage.getItem(newkeyName)
  let state = initialGridSitesState
  let storedState: NewTab.GridSitesState

  if (data) {
    try {
      storedState = JSON.parse(data)
      // add defaults for non-peristant data
      state = {
        ...state,
        ...storedState
      }
    } catch (e) {
      console.error('[GridSitesData] Could not parse local storage data: ', e)
    }
  }
  return state
}

// Saving the state is useful so that something will show when opening
// a new tab. There is a delay before MostVisitedInfoChanged() is called.
// Using `sessionStorage` won't persist to disk.
export const debouncedSave = debounce<NewTab.GridSitesState>((data: NewTab.GridSitesState) => {
  if (data) {
    window.sessionStorage.setItem(newkeyName, JSON.stringify(data))
  }
}, 50)
