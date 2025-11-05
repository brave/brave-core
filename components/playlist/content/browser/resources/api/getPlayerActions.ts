/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import store from '../store'
import { bindActionCreators, Dispatch } from 'redux'

import * as PlayerActions from '../actions/player_action_creators'

let actions: typeof PlayerActions

function getPlayerActionsForDispatch (dispatch: Dispatch) {
  return bindActionCreators(PlayerActions, dispatch)
}

// returns actions bound to dispatch.
export function getPlayerActions () {
  actions = actions || getPlayerActionsForDispatch(store.dispatch.bind(store))
  return actions
}
