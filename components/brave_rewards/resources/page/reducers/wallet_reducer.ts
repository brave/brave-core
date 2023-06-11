/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../actions/rewards_types'
import * as mojom from '../../shared/lib/mojom'
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
      const { value, error } = action.payload.result

      if (value) {
        state = { ...state, balance: optional(value.balance.total) }
      } else {
        state = { ...state, balance: optional<number>() }

        if (error === mojom.FetchBalanceError.kAccessTokenExpired) {
          chrome.send('brave_rewards.getExternalWallet')
        }
      }

      break
    }
    case types.GET_EXTERNAL_WALLET_PROVIDERS: {
      chrome.send('brave_rewards.getExternalWalletProviders')
      break
    }
    case types.SET_EXTERNAL_WALLET_TYPE: {
      chrome.send('brave_rewards.setExternalWalletType', [action.payload.provider])
      break
    }
    case types.GET_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')
      break
    }
    case types.ON_GET_EXTERNAL_WALLET: {
      const { value, error } = action.payload.result

      if (value) {
        state = { ...state, externalWallet: value.wallet }
        chrome.send('brave_rewards.fetchBalance')
      } else {
        switch (error) {
          case mojom.GetExternalWalletError.kAccessTokenExpired:
            chrome.send('brave_rewards.getExternalWallet')
            break
        }
      }

      break
    }
    case types.GET_MONTHLY_REPORT: {
      let month = action.payload.month
      let year = action.payload.year
      if (month == null) {
        month = new Date().getMonth() + 1
      }

      if (year == null) {
        year = new Date().getFullYear()
      }

      chrome.send('brave_rewards.getMonthlyReport', [month, year])
      break
    }
    case types.ON_MONTHLY_REPORT: {
      state = { ...state }
      state.monthlyReport = {
        month: action.payload.month,
        year: action.payload.year
      }

      if (!action.payload.report) {
        break
      }

      state.monthlyReport = Object.assign(state.monthlyReport, action.payload.report)
      break
    }
    case types.GET_MONTHLY_REPORT_IDS: {
      chrome.send('brave_rewards.getMonthlyReportIds')
      break
    }
    case types.ON_MONTHLY_REPORT_IDS: {
      state = { ...state }
      state.monthlyReportIds = action.payload
      break
    }
  }

  return state
}

export default walletReducer
