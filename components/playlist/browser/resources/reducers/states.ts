/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { useSelector } from 'react-redux'

// For security reason, we're embedding player to an <iframe>. And these two
// states are mutually exclusive.
export interface ApplicationState {
  // Used by app.tsx
  playlistData: PlaylistData | undefined

  // Used by a player.tsx
  playerState: PlayerState | undefined
}

export interface PlaylistData {
  lists: Playlist[]
  currentList: Playlist | undefined
  // TODO(sko) Investigate if it's possible to remove this and use ApplicationState.playerState.
  lastPlayerState: PlayerState | undefined
}

export interface PlayerState {
  currentItem: PlaylistItem | undefined
  playing: boolean
}

export const usePlaylist = (id?: string) =>
  useSelector<ApplicationState, Playlist | undefined>(applicationState =>
    applicationState.playlistData?.lists.find(e => e.id === id)
  )
