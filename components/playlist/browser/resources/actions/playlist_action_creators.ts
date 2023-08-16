/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

import {
  Playlist,
  PlaylistItem
} from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'
import {
  CachingProgress,
  PlayerState,
  PlaylistEditMode
} from '../reducers/states'

// Constants
import { types } from '../constants/playlist_types'
import {
  ItemSelectedPayload,
  SelectedPlaylistUpdatedPayload
} from '../api/playerApi'

// Actions used by App ---------------------------------------------------------
export const playlistLoaded = (playlists: Playlist[]) =>
  action(types.PLAYLIST_LOADED, playlists)

export const playlistUpdated = (playlist: Playlist) =>
  action(types.PLAYLIST_UPDATED, playlist)

// Actions used by Player ------------------------------------------------------
export const selectPlaylistItem = (payload: ItemSelectedPayload) =>
  action(types.PLAYLIST_ITEM_SELECTED, payload)

export const selectedPlaylistUpdated = (
  payload: SelectedPlaylistUpdatedPayload
) => action(types.SELECTED_PLAYLIST_UPDATED, payload)

export const cachingProgressChanged = (cachingProgress: CachingProgress) =>
  action(types.PLAYLIST_CACHING_PROGRESS_CHANGED, cachingProgress)

export const playerStateChanged = (playerState: PlayerState) =>
  action(types.PLAYLIST_PLAYER_STATE_CHANGED, playerState)

export const playerStartedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STARTED_PLAYING_ITEM)

export const playerStoppedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STOPPED_PLAYING_ITEM)

export const playNextItem = () => action(types.PLAYER_PLAY_NEXT_ITEM)

export const playPreviousItem = () => action(types.PLAYER_PLAY_PREVIOUS_ITEM)

export const setPlaylistEditMode = (editMode: PlaylistEditMode | undefined) =>
  action(types.PLAYLIST_SET_EDIT_MODE, editMode)
