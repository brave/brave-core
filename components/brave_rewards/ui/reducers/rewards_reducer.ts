/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constant
import { types } from '../constants/rewards_types'

const rewardsReducer: Reducer<Rewards.State | undefined> = (state: Rewards.State, action) => {
  switch (action.type) {
    case types.ON_SETTING_SAVE:
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        state[key] = value
        chrome.send('saveSetting', [key, value.toString()])
      }
      break
    case types.ON_MODAL_BACKUP_CLOSE:
      {
        state = { ...state }
        let ui = state.ui
        ui.walletRecoverySuccess = null
        ui.modalBackup = false
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_MODAL_BACKUP_OPEN:
      {
        let ui = state.ui
        ui.modalBackup = true
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_CLEAR_ALERT:
      {
        let ui = state.ui
        if (!ui[action.payload.property]) {
          break
        }

        ui[action.payload.property] = null
        state = {
          ...state,
          ui
        }
        break
      }
    case types.ON_RECONCILE_STAMP:
      {
        state = { ...state }
        state.reconcileStamp = parseInt(action.payload.stamp, 10)
        break
      }
  }

  return state
}

export default rewardsReducer
