/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { combineReducers } from 'redux'

import { defaultState } from './default_state'
import { loadState, saveState } from './state_cache'

import rewardsReducer from './rewards_reducer'
import walletReducer from './wallet_reducer'
import publishersReducer from './publishers_reducer'
import * as Rewards from '../lib/types'

function mergeReducers (state: Rewards.State | undefined, action: any) {
  if (!state) {
    state = loadState()
  }

  const startingState = state

  state = rewardsReducer(state, action)
  state = walletReducer(state, action)
  state = publishersReducer(state, action)

  if (!state) {
    state = defaultState()
  }

  if (state !== startingState) {
    saveState(state)
  }

  return state
}

export function createReducer () {
  return combineReducers<Rewards.ApplicationState>({
    rewardsData: mergeReducers
  })
}
