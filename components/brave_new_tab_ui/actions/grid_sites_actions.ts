// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Types
import { types } from '../constants/grid_sites_types'
import { action } from 'typesafe-actions'
import { InitialData } from '../api/initialData'

export const setFirstRenderGridSitesData = (initialData: InitialData) => {
  return action(types.GRID_SITES_SET_FIRST_RENDER_DATA, initialData)
}

export const gridSitesDataUpdated = (gridSites: NewTab.Site[]) => {
  return action(types.GRID_SITES_DATA_UPDATED, { gridSites })
}

export const toggleGridSitePinned = (pinnedSite: NewTab.Site) => {
  return action(types.GRID_SITES_TOGGLE_SITE_PINNED, { pinnedSite })
}

export const removeGridSite = (removedSite: NewTab.Site) => {
  return action(types.GRID_SITES_REMOVE_SITE, { removedSite })
}

export const undoRemoveGridSite = () => {
  return action(types.GRID_SITES_UNDO_REMOVE_SITE)
}

export const undoRemoveAllGridSites = () => {
  return action(types.GRID_SITES_UNDO_REMOVE_ALL_SITES)
}

export const addGridSites = (site: NewTab.Site) => {
  return action(types.GRID_SITES_ADD_SITES, { site })
}

export const showGridSiteRemovedNotification = (shouldShow: boolean) => {
  return action(types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION, { shouldShow })
}
