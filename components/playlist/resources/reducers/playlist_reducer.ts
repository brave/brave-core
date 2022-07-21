/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Playlist } from 'components/definitions/playlist'
import { Reducer } from 'redux'

import { types } from '../constants/playlist_types'

import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'

const playlistReducer: Reducer<Playlist.State|undefined> =
    (state: Playlist.State|undefined, action) => {
      if (state === undefined) {
        state = { lists: [] }
      }

      // TODO(sko) Handle each action properly.
      switch (action.type) {
        case types.PLAYLIST_LOADED:
          state = { ...state, lists: [...action.payload as PlaylistMojo.Playlist[]] }
          break
      }
      return state
    }

export default playlistReducer
