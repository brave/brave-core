// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  GRID_SITES_SET_FIRST_RENDER_DATA = '@@topSites/GRID_SITES_SET_FIRST_RENDER_DATA',
  GRID_SITES_DATA_UPDATED = '@@topSites/GRID_SITES_DATA_UPDATED',
  GRID_SITES_TOGGLE_SITE_PINNED = '@@topSites/GRID_SITES_SITE_PINNED',
  GRID_SITES_REMOVE_SITE = '@@topSites/GRID_SITES_REMOVE_SITE',
  GRID_SITES_UNDO_REMOVE_SITE = '@@topSites/GRID_SITES_UNDO_REMOVE_SITE',
  GRID_SITES_UNDO_REMOVE_ALL_SITES =
    '@@topSites/GRID_SITES_UNDO_REMOVE_ALL_SITES',
  GRID_SITES_UPDATE_SITE_BOOKMARK_INFO =
    '@@topSites/GRID_SITES_UPDATE_SITE_BOOKMARK_INFO',
  GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO =
    '@@topSites/GRID_SITES_TOGGLE_SITE_BOOKMARK_INFO',
  GRID_SITES_ADD_SITES = '@@topSites/GRID_SITES_ADD_SITES',
  GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION =
    '@@topSites/GRID_SITES_SHOW_SITE_REMOVED_NOTIFICATION'
}
