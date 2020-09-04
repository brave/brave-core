// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

// Types
import { types } from '../constants/grid_sites_types'
import { action } from 'typesafe-actions'

export const tilesUpdated = (gridSites: NewTab.Site[]) => {
  return action(types.GRID_SITES_DATA_UPDATED, { gridSites })
}

export const showTilesRemovedNotice = (shouldShow: boolean) => {
  return action(types.GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION, { shouldShow })
}
