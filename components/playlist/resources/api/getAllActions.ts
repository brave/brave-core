/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import store from '../store'

import * as PlaylistActions from '../actions/playlist_action_creators'
import { bindActionCreators, Dispatch } from 'redux'

type Action = typeof PlaylistActions

let actions: Action

// returns actions bound to dispatch.
export function getAllActions () {
   actions = actions || getActionsForDispatch(store.dispatch.bind(store))
   return actions
}

export function getActionsForDispatch (dispatch: Dispatch) {
    return bindActionCreators(PlaylistActions, dispatch)
}
