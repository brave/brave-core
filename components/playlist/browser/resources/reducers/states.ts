/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

export interface ApplicationState {
  playlistData: PlaylistData | undefined
  playerState: PlayerState | undefined
}

export interface PlaylistData {
  lists: PlaylistMojo.Playlist[]
  currentList: PlaylistMojo.Playlist | undefined
  lastPlayerState: PlayerState | undefined
}

export interface PlayerState {
  currentItem: PlaylistMojo.PlaylistItem | undefined
  playing: boolean
}
