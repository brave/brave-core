/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

import * as PlaylistMojo from 'gen/brave/components/playlist/mojom/playlist.mojom.m.js'

// Constants
import { types } from '../constants/playlist_types'

export const playlistLoaded = (playlists: PlaylistMojo.Playlist[]) =>
    action(types.PLAYLIST_LOADED, playlists)

export const selectPlaylist = (playlist: PlaylistMojo.Playlist) =>
    action(types.PLAYLIST_SELECTED, playlist)
