// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export const enum types {
  TILES_UPDATED = '@@topSites/TILES_UPDATED',
  TILE_REMOVED = '@@topSites/TILE_REMOVED',
  TILES_REORDERED = '@@topSites/TILES_REORDERED',
  RESTORE_DEFAULT_TILES = '@@topSites/RESTORE_DEFAULT_TILES',
  SHOW_TILES_REMOVED_NOTICE =
    '@@topSites/SHOW_TILES_REMOVED_NOTICE',
  UNDO_REMOVE_TILE = '@@topSites/UNDO_REMOVE_TILE'
}
