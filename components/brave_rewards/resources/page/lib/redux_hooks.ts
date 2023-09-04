/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { bindActionCreators } from 'redux'
import { useDispatch, useSelector } from 'react-redux'
import * as rewardsActions from '../actions/rewards_actions'
import { defaultState } from '../reducers/default_state'
import * as Rewards from './types'

export function useActions () {
  return bindActionCreators(rewardsActions, useDispatch())
}

type StateSelector<T> = (state: Rewards.State) => T

export function useRewardsData<T> (fn: StateSelector<T>) {
  return useSelector((state: Rewards.ApplicationState) => {
    return fn(state.rewardsData || defaultState())
  })
}
