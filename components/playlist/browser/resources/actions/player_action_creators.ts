/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

import { types } from '../constants/player_types'

import {
  ItemSelectedPayload,
  SelectedPlaylistUpdatedPayload
} from '../api/playerApi'

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'

export const selectPlaylistItem = (payload: ItemSelectedPayload) =>
  action(types.PLAYLIST_ITEM_SELECTED, payload)

export const selectedPlaylistUpdated = (
  payload: SelectedPlaylistUpdatedPayload
) => action(types.SELECTED_PLAYLIST_UPDATED, payload)

export const playerStartedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STARTED_PLAYING_ITEM)

export const playerStoppedPlayingItem = (playlist: PlaylistItem | undefined) =>
  action(types.PLAYER_STOPPED_PLAYING_ITEM)

export const playNextItem = () => action(types.PLAYER_PLAY_NEXT_ITEM)

export const playPreviousItem = () => action(types.PLAYER_PLAY_PREVIOUS_ITEM)

export const toggleShuffle = () => action(types.PLAYER_TOGGLE_SHUFFLE)

export const advanceLoopMode = () => action(types.PLAYER_ADVANCE_LOOP_MODE)

export const toggleAutoPlay = () => action(types.PLAYER_TOGGLE_AUTO_PLAY)

export const unloadPlaylist = () => action(types.UNLOAD_PLAYLIST)
