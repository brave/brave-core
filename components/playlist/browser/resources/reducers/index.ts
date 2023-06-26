/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ApplicationState } from 'components/playlist/browser/resources/reducers/states'
import { combineReducers } from 'redux'

// Utils
import playerReducer from './player_reducer'
import playlistReducer from './playlist_reducer'

export default combineReducers<ApplicationState>({
  playlistData: playlistReducer,
  playerState: playerReducer
})
