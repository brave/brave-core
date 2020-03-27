// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Redux API
import { Reducer } from 'redux'

// Types
import { types } from '../constants/grid_sites_types'

// API
import * as gridSitesState from '../state/gridSitesState'
import * as storage from '../storage/grid_sites_storage'

const initialState = storage.load()

export const gridSitesReducer: Reducer<NewTab.GridSitesState | undefined> = (
  state: NewTab.GridSitesState | undefined,
  action
) => {
  if (state === undefined) {
    state = initialState
  }

  const payload = action.payload
  const startingState = state

  switch (action.type) {
    case types.GRID_SITES_SET_FIRST_RENDER_DATA: {

      // If there are legacy values from a previous
      // storage, update first render data with it
      state = gridSitesState
        .gridSitesReducerSetFirstRenderDataFromLegacy(
          state,
          startingState.legacy
        )
      // Now that we stored the legacy reference, delete it
      // so it won't override gridSites in further updates
      if (startingState.legacy) {
        delete startingState.legacy
      }

      // New profiles just store what comes from Chromium
      state = gridSitesState
        .gridSitesReducerSetFirstRenderData(state, payload.topSites)

      // Handle default top sites data only once.
      if (payload.defaultTopSites && !storage.isDefaultTopSitesAddedToPinnedSites()) {
        state = gridSitesState
          .gridSitesReducerSetDefaultTopSites(state, payload.defaultTopSites)
        storage.setDefaultTopSitesAddedToPinnedSites()
      }
      break
    }

    case types.GRID_SITES_DATA_UPDATED: {
      state = gridSitesState
        .gridSitesReducerDataUpdated(state, payload.gridSites)
      break
    }

    case types.GRID_SITES_TOGGLE_SITE_PINNED: {
      state = gridSitesState
        .gridSitesReducerToggleSitePinned(state, payload.pinnedSite)
      break
    }

    case types.GRID_SITES_REMOVE_SITE: {
      state = gridSitesState
        .gridSitesReducerRemoveSite(state, payload.removedSite)
      break
    }

    case types.GRID_SITES_UNDO_REMOVE_SITE: {
      state = gridSitesState
        .gridSitesReducerUndoRemoveSite(state)
      break
    }

    case types.GRID_SITES_UNDO_REMOVE_ALL_SITES: {
      state = gridSitesState
        .gridSitesReducerUndoRemoveAllSites(state)
      break
    }

    case types.GRID_SITES_UPDATE_SITE_BOOKMARK_INFO: {
      state = gridSitesState
        .gridSitesReducerUpdateSiteBookmarkInfo(state, payload.bookmarkInfo)
      break
    }

    case types.GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO: {
      state = gridSitesState
        .gridSitesReducerToggleSiteBookmarkInfo(
          state,
          payload.url,
          payload.bookmarkInfo
        )
      break
    }

    case types.GRID_SITES_ADD_SITES: {
      state = gridSitesState.gridSitesReducerAddSiteOrSites(state, payload.site)
      break
    }

    case types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION: {
      state = gridSitesState
        .gridSitesReducerShowSiteRemovedNotification(state, payload.shouldShow)
      break
    }
  }

  if (JSON.stringify(state) !== JSON.stringify(startingState)) {
    storage.debouncedSave(state)
  }

  return state
}

export default gridSitesReducer
