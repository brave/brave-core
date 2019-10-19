/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const createWallet = (state: Rewards.State) => {
  state.walletCreated = true
  state.enabledMain = true
  state.enabledAds = true
  state.enabledContribute = true
  state.createdTimestamp = new Date().getTime()

  chrome.send('brave_rewards.getReconcileStamp')

  return state
}

const walletReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.CREATE_WALLET:
      state = { ...state }
      state.walletCreateFailed = false
      state.walletCreated = false
      chrome.send('brave_rewards.createWalletRequested')
      break
    case types.WALLET_CREATED:
      state = { ...state }
      state = createWallet(state)
      chrome.send('brave_rewards.saveAdsSetting', ['adsEnabled', 'true'])
      break
    case types.WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      break
    case types.GET_WALLET_PROPERTIES:
      chrome.send('brave_rewards.getWalletProperties')
      break
    case types.ON_WALLET_PROPERTIES: {
      state = { ...state }
      let ui = state.ui

      // TODO NZ check why enum can't be used inside Rewards namespace
      if (action.payload.properties.status === 1) {
        ui.walletServerProblem = true
      } else if (action.payload.properties.status === 17) {
        ui.walletCorrupted = true
      } else {
        state.contributionMonthly = action.payload.properties.monthlyAmount
        state.walletInfo = action.payload.properties.wallet
        ui.walletServerProblem = false
        ui.walletCorrupted = false
      }

      state = {
        ...state,
        ui
      }
      break
    }
    case types.GET_WALLLET_PASSPHRASE:
      chrome.send('brave_rewards.getWalletPassphrase')
      break
    case types.ON_WALLLET_PASSPHRASE:
      const value = action.payload.pass
      if (value && value.length > 0) {
        state = { ...state }
        let ui = state.ui
        state.recoveryKey = value
        ui.paymentIdCheck = true

        state = {
          ...state,
          ui
        }
      }
      break
    case types.RECOVER_WALLET: {
      let key = action.payload.key
      key = key.trim()

      if (!key || key.length === 0) {
        let ui = state.ui
        ui.walletRecoverySuccess = false

        state = {
          ...state,
          ui
        }

        break
      }

      chrome.send('brave_rewards.recoverWallet', [key])
      break
    }
    case types.ON_RECOVER_WALLET_DATA: {
      state = { ...state }
      const result = action.payload.properties.result
      let ui = state.ui
      let balance = state.balance

      // TODO NZ check why enum can't be used inside Rewards namespace
      ui.walletRecoverySuccess = result === 0
      if (result === 0) {
        balance.total = action.payload.properties.balance
        chrome.send('brave_rewards.getWalletPassphrase')
        chrome.send('brave_rewards.getGrants', ['', ''])
        ui.emptyWallet = balance.total <= 0
        ui.modalBackup = false
        ui.walletCorrupted = false
      }

      state = {
        ...state,
        ui,
        balance
      }
      break
    }
    case types.GET_CURRENT_REPORT: {
      chrome.send('brave_rewards.getBalanceReports')
      break
    }
    case types.ON_BALANCE_REPORTS: {
      state = { ...state }
      state.reports = action.payload.reports
      break
    }
    case types.CHECK_WALLET_EXISTENCE: {
      chrome.send('brave_rewards.checkWalletExistence')
      break
    }
    case types.ON_WALLET_EXISTS: {
      if (!action.payload.exists || state.walletCreated) {
        break
      }
      state = { ...state }
      state = createWallet(state)
      state.firstLoad = false
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
        action.payload.publisherKey,
        action.payload.viewingId,
        action.payload.addedDate
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

      if (status === 0) { // on ledger::Result::LEDGER_OK
        state.balance = action.payload.balance
        ui.walletServerProblem = false

        if (ui.emptyWallet && state.balance && state.balance.total > 0) {
          ui.emptyWallet = false
        }
      } else if (status === 1) { // on ledger::Result::LEDGER_ERROR
        ui.walletServerProblem = true
      } else if (status === 24) { // on ledger::Result::EXPIRED_TOKEN
        chrome.send('brave_rewards.getExternalWallet', ['uphold'])
      }

      state = {
        ...state,
        ui
      }
      break
    }
    case types.GET_EXTERNAL_WALLET: {
      chrome.send('brave_rewards.getExternalWallet', [action.payload.type])
      break
    }
    case types.ON_EXTERNAL_WALLET: {
      state = { ...state }

      if (action.payload.result === 24) { // on ledger::Result::EXPIRED_TOKEN
        chrome.send('brave_rewards.getExternalWallet', ['uphold'])
        break
      }

      state.externalWallet = action.payload.wallet
    }
  }

  return state
}

export default walletReducer
