/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { PlaylistData } from 'components/playlist/browser/resources/reducers/states'
import { Reducer } from 'redux'

import { types } from '../constants/playlist_types'

import { Playlist } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { fixUpMediaURL, fixUpThumbnailURL } from '../utils/urlFixer'

const playlistReducer: Reducer<PlaylistData | undefined> = (
  state: PlaylistData | undefined,
  action
) => {
  if (state === undefined) {
    state = { lists: [], currentList: undefined, lastPlayerState: undefined }
  }

  switch (action.type) {
    case types.PLAYLIST_LOADED:
      const playlists = action.payload as Playlist[]

      // Fix up local thumbnail url to chrome-untrusted
      if (location.protocol === 'chrome-untrusted:') {
        for (const playlist of playlists) {
          for (const item of playlist.items) {
            fixUpThumbnailURL(item)
            fixUpMediaURL(item)
          }
        }
      }

      let currentList: Playlist | undefined
      if (state.currentList) {
        currentList = playlists.find((list: Playlist) => {
          return list.id === state?.currentList?.id
        })
      }
      if (!currentList) currentList = playlists[0]

      state = { ...state, currentList, lists: [...playlists] }
      break

    case types.PLAYLIST_SELECTED:
      const playlist = action.payload
      state = { ...state, currentList: playlist }
      break

    case types.PLAYLIST_PLAYER_STATE_CHANGED:
      state = { ...state, lastPlayerState: action.payload }
      break
  }
  return state
}

export default playlistReducer
