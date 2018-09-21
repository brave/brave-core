/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../../common/debounce'
import { RewardsPanelState } from '../constants/rewardsPanelState'

const keyName = 'rewards-panel-data'

const defaultState: RewardsPanelState = {
  walletCreated: false,
  walletCreateFailed: false,
  publisher: undefined
}

export const load = (): RewardsPanelState => {
  const data = window.localStorage.getItem(keyName)
  let state: RewardsPanelState = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return state
}

export const debouncedSave = debounce((data: RewardsPanelState) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(data))
  }
}, 50)
