/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Utils
import { debounce } from '../../../../../common/debounce'

const keyName = 'rewards-panel-data'

export const defaultState: RewardsExtension.State = {
  walletCorrupted: false,
  walletCreated: false,
  walletCreating: false,
  walletCreateFailed: false,
  publishers: {},
  parameters: {
    monthlyTipChoices: [],
    rate: 0
  },
  balanceReport: {
    ads: 0.0,
    contribute: 0.0,
    monthly: 0.0,
    grant: 0.0,
    tips: 0.0
  },
  notifications: {},
  currentNotification: undefined,
  pendingContributionTotal: 0,
  enabledMain: false,
  enabledAC: false,
  promotions: [],
  recurringTips: [],
  tipAmounts: {},
  balance: {
    total: 0,
    wallets: {}
  },
  initializing: true
}

const cleanData = (state: RewardsExtension.State) => {
  state = { ...state }
  state.publishers = {}

  const balance = state.balance as any
  if (!balance || balance.total == null) {
    state.balance = defaultState.balance
  }

  if (!state.parameters) {
    state.parameters = defaultState.parameters
  }

  return state
}

export const load = (): RewardsExtension.State => {
  const data = window.localStorage.getItem(keyName)
  let state: RewardsExtension.State = defaultState
  if (data) {
    try {
      state = JSON.parse(data)
      state.initializing = true
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
