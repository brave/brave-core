// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { Playlist } from 'components/definitions/playlist'
import { Reducer } from 'redux'

import { types } from '../constants/playlist_types'

import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'

const playerReducer: Reducer<Playlist.PlayerState|undefined> =
    (state: Playlist.PlayerState|undefined, action) => {
      if (state === undefined) {
        state = { currentItem: undefined, playing: false }
      }

      switch (action.type) {
        case types.PLAYLIST_ITEM_SELECTED:
          const playlistItem = action.payload as PlaylistMojo.PlaylistItem
          state = { ...state, currentItem: playlistItem }
          break

        case types.PLAYER_STARTED_PLAYING_ITEM:
          state = { ...state, playing: true }
          break

        case types.PLAYER_STOPPED_PLAYING_ITEM:
          state = { ...state, playing: false }
          break
      }
      return state
    }

export default playerReducer
