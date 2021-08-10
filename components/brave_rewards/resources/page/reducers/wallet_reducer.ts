/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

import { getCurrentBalanceReport } from '../utils'

// Constant
import { types } from '../constants/rewards_types'

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
    case types.RECOVER_WALLET: {
      let key = action.payload.key
      key = key.trim()

      if (!key || key.length === 0) {
        let ui = state.ui
        ui.walletRecoveryStatus = 0

        state = {
          ...state,
          ui
        }

        break
      }

      chrome.send('brave_rewards.recoverWallet', [key])
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
    case types.ON_RECOVER_WALLET_DATA: {
      state = { ...state }
      const result = action.payload.result
      let ui = state.ui

      // TODO NZ check why enum can't be used inside Rewards namespace
      ui.walletRecoveryStatus = result
      if (result === 0) {
        chrome.send('brave_rewards.fetchPromotions')
        chrome.send('brave_rewards.fetchBalance')
        chrome.send('brave_rewards.getPaymentId')
        getCurrentBalanceReport()
        ui.modalBackup = false
        ui.emptyWallet = false
        state.recoveryKey = ''
      }

      state = {
        ...state,
        ui
      }
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
    case types.GET_PENDING_CONTRIBUTIONS: {
      chrome.send('brave_rewards.getPendingContributions')
      break
    }
    case types.ON_PENDING_CONTRIBUTIONS: {
      state = { ...state }
      state.pendingContributions = action.payload.list
      const total = state.pendingContributions
        .reduce((accumulator: number, item: Rewards.PendingContribution) => {
          return accumulator + item.amount
        }, 0)
      state.pendingContributionTotal = total
      break
    }
    case types.REMOVE_PENDING_CONTRIBUTION: {
      chrome.send('brave_rewards.removePendingContribution', [
        action.payload.id
      ])
      break
    }
    case types.REMOVE_ALL_PENDING_CONTRIBUTION: {
      chrome.send('brave_rewards.removeAllPendingContribution')
      break
    }
    case types.GET_BALANCE: {
      chrome.send('brave_rewards.fetchBalance')
      break
    }
    case types.ON_BALANCE: {
      const status = action.payload.status
      let ui = state.ui

      if (status === 0) { // on ledger::type::Result::LEDGER_OK
        state.balance = action.payload.balance
        ui.walletServerProblem = false

        if (ui.emptyWallet && state.balance && state.balance.total > 0) {
          ui.emptyWallet = false
        }
      } else if (status === 1) { // on ledger::type::Result::LEDGER_ERROR
        ui.walletServerProblem = true
      } else if (status === 24) { // on ledger::type::Result::EXPIRED_TOKEN
        chrome.send('brave_rewards.getExternalWallet')
        state.balance.total = action.payload.balance.total || 0
      }

      state = {
        ...state,
        ui
      }
      break
    }
    case types.GET_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet')
      break
    }
    case types.ON_EXTERNAL_WALLET: {
      state = { ...state }

      if (action.payload.result === 24) { // on ledger::type::Result::EXPIRED_TOKEN
        chrome.send('brave_rewards.getExternalWallet')
        break
      }

      if (action.payload.result === 0) { // type::Result::LEDGER_OK
        chrome.send('brave_rewards.fetchBalance')
      }

      state.externalWallet = action.payload.wallet
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
    case types.GET_WALLET_PASSPHRASE:	{
      chrome.send('brave_rewards.getWalletPassphrase')
      break
    }
    case types.ON_WALLET_PASSPHRASE: {
      const value = action.payload.passphrase
      if (value && value.length > 0) {
        state = { ...state }
        let ui = state.ui
        state.recoveryKey = value

        state = {
          ...state,
          ui
        }
      }
      break
    }
  }

  return state
}

export default walletReducer
