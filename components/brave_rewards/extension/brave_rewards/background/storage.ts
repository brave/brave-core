/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../../common/debounce'

const keyName = 'rewards-panel-data'

const defaultState: RewardsExtension.State = {
  walletCreated: false,
  walletCreateFailed: false,
  publishers: {},
  walletProperties: {
    balance: 0,
    probi: '0',
    grants: [],
    rates: {}
  },
  report: {
    ads: 0,
    closing: 0,
    contribute: 0,
    donations: 0,
    grants: 0,
    oneTime: 0,
    opening: 0,
    total: 0
  }
}

const cleanData = (state: RewardsExtension.State) => {
  state = { ...state }
  state.publishers = {}
  return state
}

export const load = (): RewardsExtension.State => {
  const data = window.localStorage.getItem(keyName)
  let state: RewardsExtension.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
    } catch (e) {
      console.error('Could not parse local storage data: ', e)
    }
  }
  return cleanData(state)
}

export const debouncedSave = debounce((data: RewardsExtension.State) => {
  if (data) {
    window.localStorage.setItem(keyName, JSON.stringify(cleanData(data)))
  }
}, 50)
