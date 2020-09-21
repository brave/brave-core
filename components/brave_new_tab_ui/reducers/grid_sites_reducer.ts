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
import {
  deleteMostVisitedTile,
  reorderMostVisitedTile,
  restoreMostVisitedDefaults,
  setMostVisitedSettings,
  undoMostVisitedTileAction
} from '../api/topSites'

// Utils
import arrayMove from 'array-move'

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
    case types.GRID_SITES_DATA_UPDATED: {
      const { gridSites, customLinksEnabled, visible } = payload
      setMostVisitedSettings(customLinksEnabled, visible, false)
      state = gridSitesState.tilesUpdated(state, gridSites)
      break
    }

    case types.GRID_SITES_REMOVE: {
      const { url } = payload
      deleteMostVisitedTile(url)
      state = gridSitesState.showTilesRemovedNotice(state, true)
      break
    }

    case types.GRID_SITES_REORDER: {
      const { gridSites, oldPos, newPos } = payload
      // "Super referral" entries (if present) are always at the beginning
      // Skip these indices when determining the new position for Chromium
      let offset: number = 0
      for (let i: number = 0; i < gridSites.length; i++) {
        if (!gridSites[i].defaultSRTopSite) {
          break
        }
        offset++
      }
      // Change the order in Chromium
      reorderMostVisitedTile(gridSites[oldPos].url, (newPos - offset))
      // Change the order that user sees. Chromium will overwrite this
      // when `MostVisitedInfoChanged` is called- but changing BEFORE that
      // avoids a flicker for the user where (for a second or so), tiles would
      // have the wrong order.
      const reorderedGridSites: NewTab.Site[] =
          arrayMove(gridSites, oldPos, newPos)
      state = gridSitesState.tilesUpdated(state, reorderedGridSites)
      break
    }

    case types.GRID_SITES_RESTORE_DEFAULTS: {
      restoreMostVisitedDefaults()
      state = gridSitesState.showTilesRemovedNotice(state, false)
      break
    }

    case types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION: {
      state = gridSitesState.showTilesRemovedNotice(state, payload.shouldShow)
      break
    }

    case types.GRID_SITES_UNDO_ACTION: {
      undoMostVisitedTileAction()
      state = gridSitesState.showTilesRemovedNotice(state, false)
    }
  }

  if (JSON.stringify(state) !== JSON.stringify(startingState)) {
    storage.debouncedSave(state)
  }

  return state
}

export default gridSitesReducer
