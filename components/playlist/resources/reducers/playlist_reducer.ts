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
        state = { lists: [], currentList: undefined }
      }

      switch (action.type) {
        case types.PLAYLIST_LOADED:
          const playlists = action.payload
          let currentList: PlaylistMojo.Playlist | undefined
          if (state.currentList) {
            currentList = playlists.find((list: PlaylistMojo.Playlist) => list.id === state?.currentList?.id)
          }
          if (!currentList) currentList = playlists[0]

          if (playlists.length && !currentList) {
            console.error('there\'s no selected playlist even though we have playlists')
          }

          state = { ...state, currentList, lists: [...playlists] }
          break

        case types.PLAYLIST_SELECTED:
          const playlist = action.payload
          state = { ...state, currentList: playlist }
      }
      return state
    }

export default playlistReducer
