/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../actions/rewards_types'
import {
  optional
} from '../../../../brave_rewards/resources/shared/lib/optional'
import * as Rewards from '../lib/types'

const walletReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  if (!state) {
    return
  }

  switch (action.type) {
    case types.GET_REWARDS_PARAMETERS:
      chrome.send('brave_rewards.getRewardsParameters')
      break
    case types.ON_REWARDS_PARAMETERS: {
      state = { ...state }
      state.parameters = action.payload.properties
      break
    }
    case types.ON_EXTERNAL_WALLET_PROVIDER_LIST: {
      if (!action.payload.list) {
        break
      }

      state = { ...state }
      state.externalWalletProviderList = action.payload.list
      break
    }
    case types.GET_BALANCE_REPORT: {
      chrome.send('brave_rewards.getBalanceReport', [
        action.payload.month,
        action.payload.year
      ])
      break
    }
    case types.ON_BALANCE_REPORT: {
      state = { ...state }
      state.balanceReport = action.payload.report
      break
    }
    case types.GET_CONTRIBUTION_AMOUNT: {
      chrome.send('brave_rewards.getContributionAmount')
      break
    }
    case types.ON_CONTRIBUTION_AMOUNT: {
      state = { ...state }
      state.contributionMonthly = action.payload.amount
      break
    }
    case types.GET_RECONCILE_STAMP: {
      chrome.send('brave_rewards.getReconcileStamp')
      break
    }
    case types.GET_BALANCE: {
      chrome.send('brave_rewards.fetchBalance')
      break
    }
    case types.ON_BALANCE: {
      const { balance } = action.payload
      if (balance) {
        state = { ...state, balance: optional(balance.total) }
      } else {
        state = { ...state, balance: optional<number>() }
      }
      break
    }
    case types.GET_EXTERNAL_WALLET_PROVIDERS: {
      chrome.send('brave_rewards.getExternalWalletProviders')
      break
    }
    case types.BEGIN_EXTERNAL_WALLET_LOGIN: {
      const { provider } = action.payload
      chrome.send('brave_rewards.beginExternalWalletLogin', [provider])
      state = {
        ...state,
        ui: { ...state.ui, modalConnectState: 'loading' }
      }
      break
    }
    case types.ON_EXTERNAL_WALLET_LOGIN_ERROR: {
      state = {
        ...state,
        ui: { ...state.ui, modalConnectState: 'error' }
      }
      break
    }
    case types.GET_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')
      break
    }
    case types.ON_GET_EXTERNAL_WALLET: {
      const { externalWallet } = action.payload
      if (externalWallet) {
        state = { ...state, externalWallet }
        chrome.send('brave_rewards.fetchBalance')
      }
      break
    }
  }

  return state
}

export default walletReducer
