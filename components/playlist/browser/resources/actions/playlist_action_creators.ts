/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import * as States from 'components/playlist/browser/resources/reducers/states'

// Constants
import { types } from '../constants/playlist_types'

export const playlistLoaded = (playlists: PlaylistMojo.Playlist[]) =>
  action(types.PLAYLIST_LOADED, playlists)

export const selectPlaylist = (playlist: PlaylistMojo.Playlist) =>
  action(types.PLAYLIST_SELECTED, playlist)

export const selectPlaylistItem = (playlist: PlaylistMojo.PlaylistItem) =>
  action(types.PLAYLIST_ITEM_SELECTED, playlist)

export const playerStateChanged = (playerState: States.PlayerState) =>
  action(types.PLAYLIST_PLAYER_STATE_CHANGED, playerState)

export const playerStartedPlayingItem = (
  playlist: PlaylistMojo.PlaylistItem | undefined
) => action(types.PLAYER_STARTED_PLAYING_ITEM)

export const playerStoppedPlayingItem = (
  playlist: PlaylistMojo.PlaylistItem | undefined
) => action(types.PLAYER_STOPPED_PLAYING_ITEM)
