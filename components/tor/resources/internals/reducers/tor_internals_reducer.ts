/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/tor_internals_types'

// Utils
import * as storage from '../storage'

const torInternalsReducer: Reducer<TorInternals.State | undefined> = (state: TorInternals.State | undefined, action) => {
  if (state === undefined) {
    state = storage.load()
  }

  switch (action.type) {
    case types.GET_TOR_GENERAL_INFO:
      chrome.send('tor_internals.getTorGeneralInfo')
      break
    case types.ON_GET_TOR_GENERAL_INFO:
      state = {
        ...state,
        generalInfo: action.payload.generalInfo
      }
      break
    case types.GET_TOR_LOG:
      chrome.send('tor_internals.getTorLog')
      break
    case types.ON_GET_TOR_LOG:
      state = {
        ...state,
        log: action.payload.log
      }
      break
    case types.ON_GET_TOR_INIT_PERCENTAGE:
      state = {
        ...state,
        generalInfo : {
          ...state.generalInfo,
          torInitPercentage: action.payload.percentage
        }
      }
      break
    case types.ON_GET_TOR_CIRCUIT_ESTABLISHED:
      state = {
        ...state,
        generalInfo : {
          ...state.generalInfo,
          isTorConnected: action.payload.success
        }
      }
      break
    case types.ON_GET_TOR_CONTROL_EVENT:
      state = { ...state }
      state.torControlEvents.push(action.payload.event)
      break
    default:
      break
  }

  return state
}

export default torInternalsReducer
