// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Types
import { types } from '../constants/grid_sites_types'
import { action } from 'typesafe-actions'

export const tilesUpdated = (gridSites: NewTab.Site[],
    customLinksEnabled: boolean, visible: boolean) => {
  return action(types.GRID_SITES_DATA_UPDATED, { gridSites,
    customLinksEnabled, visible })
}

export const tileRemoved = (url: string) => {
  return action(types.GRID_SITES_REMOVE, { url })
}

export const tilesReordered = (gridSites: NewTab.Site[],
    oldPos: number, newPos: number) => {
  return action(types.GRID_SITES_REORDER, { gridSites, oldPos, newPos })
}

export const restoreDefaultTiles = () => {
  return action(types.GRID_SITES_RESTORE_DEFAULTS, {})
}

export const showTilesRemovedNotice = (shouldShow: boolean) => {
  return action(types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION, { shouldShow })
}

export const undoRemoveTile = () => {
  return action(types.GRID_SITES_UNDO_ACTION, {})
}
