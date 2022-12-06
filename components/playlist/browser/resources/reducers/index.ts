/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Playlist } from 'components/definitions/playlist'
import { combineReducers } from 'redux'

import playerReducer from './player_reducer'
// Utils
import playlistReducer from './playlist_reducer'

export default combineReducers<Playlist.ApplicationState>(
    { playlistData: playlistReducer, playerState: playerReducer })
