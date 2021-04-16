/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/playlist_types'

export const playlistItemStatusChanged = (id: string) =>
  action(types.PLAYLIST_ITEM_STATUS_CHANGED, { id })
