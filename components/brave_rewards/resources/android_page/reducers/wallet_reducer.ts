/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const walletReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.GET_REWARDS_PARAMETERS:
      chrome.send('brave_rewards.getRewardsParameters')
      break
    case types.ON_REWARDS_PARAMETERS: {
      state = { ...state }
      let ui = state.ui

      // TODO NZ check why enum can't be used inside Rewards namespace
      if (action.payload.properties.status === 1) {
        ui.walletServerProblem = true
      } else {
        state.parameters = action.payload.properties
        ui.walletServerProblem = false
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
      }

      state = {
        ...state,
        ui
      }
      break
    }
  }

  return state
}

export default walletReducer
