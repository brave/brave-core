/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/rewards_internals_types'

// Utils
import * as storage from '../storage'

const rewardsInternalsReducer: Reducer<RewardsInternals.State | undefined> = (state: RewardsInternals.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  switch (action.type) {
    case types.GET_REWARDS_INTERNALS_INFO:
      chrome.send('brave_rewards_internals.getRewardsInternalsInfo')
      break
    case types.ON_GET_REWARDS_INTERNALS_INFO:
      state = { ...state }
      state.info = action.payload.info
      break
    case types.GET_BALANCE:
      chrome.send('brave_rewards_internals.getBalance')
      break
    case types.ON_BALANCE:
      state = { ...state }
      state.balance = action.payload.balance
      break
    case types.GET_CONTRIBUTIONS:
      chrome.send('brave_rewards_internals.getContributions')
      break
    case types.ON_CONTRIBUTIONS:
      state = { ...state }
      state.contributions = action.payload.contributions
      break
    case types.GET_PARTIAL_LOG:
      chrome.send('brave_rewards_internals.getPartialLog')
      break
    case types.ON_GET_PARTIAL_LOG:
      state = { ...state }
      state.log = action.payload.log
      break
    case types.GET_FULL_LOG:
      chrome.send('brave_rewards_internals.getFullLog')
      break
    case types.ON_GET_FULL_LOG:
      state = { ...state }
      state.fullLog = action.payload.log
      break
    case types.CLEAR_LOG:
      chrome.send('brave_rewards_internals.clearLog')
      break
    case types.DOWNLOAD_COMPLETED:
      state.fullLog = ''
      break
    case types.GET_EXTERNAL_WALLET:
      chrome.send('brave_rewards_internals.getExternalWallet')
      break
    case types.ON_GET_EXTERNAL_WALLET:
      state = { ...state }
      state.externalWallet = action.payload.wallet
      break
    case types.GET_EVENT_LOGS:
      chrome.send('brave_rewards_internals.getEventLogs')
      break
    case types.ON_EVENT_LOGS:
      state = { ...state }
      if (!action.payload.logs || !Array.isArray(action.payload.logs)) {
        break
      }
      state.eventLogs = action.payload.logs
        .sort((a: RewardsInternals.EventLog, b: RewardsInternals.EventLog) => b.createdAt - a.createdAt)
      break
    case types.GET_AD_DIAGNOSTICS:
      chrome.send('brave_rewards_internals.getAdDiagnostics')
      break
    case types.ON_AD_DIAGNOSTICS:
      state = { ...state }
      state.adDiagnostics = action.payload.adDiagnostics
      break
    case types.SET_AD_DIAGNOSTIC_ID:
      chrome.send('brave_rewards_internals.setAdDiagnosticId', [action.payload.adDiagnosticId])
      state = {
        ...state,
        adDiagnostics: {
          ...state.adDiagnostics,
          diagnosticId: action.payload.adDiagnosticId
        }
      }
      break
    case types.GET_ENVIRONMENT:
      chrome.send('brave_rewards_internals.getEnvironment')
      break
    case types.ON_ENVIRONMENT:
      state = { ...state }
      state.environment = action.payload.environment
      break
    default:
      break
  }

  return state
}

export default rewardsInternalsReducer
