/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import store from '../store'

import * as PlaylistActions from '../actions/playlist_action_creators'
import { bindActionCreators, Dispatch } from 'redux'

let actions: typeof PlaylistActions

function getPlaylistActionsForDispatch (dispatch: Dispatch) {
  return bindActionCreators(PlaylistActions, dispatch)
}

// returns actions bound to dispatch.
export function getPlaylistActions () {
  actions = actions || getPlaylistActionsForDispatch(store.dispatch.bind(store))
  return actions
}
