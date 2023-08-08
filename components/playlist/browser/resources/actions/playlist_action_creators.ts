/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'
import { CachingProgress, PlayerState } from '../reducers/states'

// Constants
import { types } from '../constants/playlist_types'

export const playlistLoaded = (playlists: Playlist[]) =>
  action(types.PLAYLIST_LOADED, playlists)

export const selectPlaylist = (playlist: Playlist) =>
  action(types.PLAYLIST_SELECTED, playlist)

export const selectPlaylistItem = (playlist: PlaylistItem) =>
  action(types.PLAYLIST_ITEM_SELECTED, playlist)

export const cachingProgressChanged = (cachingProgress: CachingProgress) =>
  action(types.PLAYLIST_CACHING_PROGRESS_CHANGED, cachingProgress)

export const playerStateChanged = (playerState: PlayerState) =>
  action(types.PLAYLIST_PLAYER_STATE_CHANGED, playerState)

export const playerStartedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STARTED_PLAYING_ITEM)

export const playerStoppedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STOPPED_PLAYING_ITEM)
