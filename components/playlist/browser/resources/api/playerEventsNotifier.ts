// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'
import { types } from '../constants/playlist_types'
import { PlayerState } from '../reducers/states'
export { types as PlaylistTypes } from '../constants/playlist_types'

type PlayerStateChangedPayload = {
  type: types.PLAYLIST_PLAYER_STATE_CHANGED
  data: PlayerState
}

type LastPlayedPositionChangedPayload = {
  type: types.PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED
  data: PlaylistItem
}

type GoBackToCurrentFolderPayload = {
  type: types.PLAYLIST_GO_BACK_TO_CURRENTLY_PLAYING_FOLDER
  data: { currentList: Playlist; currentItem: PlaylistItem }
}

type OpenSourcePagePayload = {
  type: types.PLAYLIST_OPEN_SOURCE_PAGE
  data: PlaylistItem
}

type FailedToPlayItemPayload = {
  type: types.PLAYLIST_PLAYER_FAILED_TO_PLAY_ITEM
  data: PlaylistItem
}

export type PlayerEventsPayload =
  | PlayerStateChangedPayload
  | LastPlayedPositionChangedPayload
  | GoBackToCurrentFolderPayload
  | OpenSourcePagePayload
  | FailedToPlayItemPayload

export function notifyEventsToTopFrame(playerState: PlayerEventsPayload) {
  if (location.protocol.startsWith('chrome-untrusted:')) {
    window.parent.postMessage(playerState, 'chrome-untrusted://playlist')
  }
}
