/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/welcome_types'

// Utils
import * as storage from '../storage'

const welcomeReducer: Reducer<Welcome.State | undefined> = (state: Welcome.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
  }

  const payload = action.payload
  const startingState = state
  switch (action.type) {
    case types.IMPORT_BROWSER_THEMES_SUCCESS:
      state = { ...state, browserThemes: payload }
      break
    case types.SET_BROWSER_THEME:
      chrome.braveTheme.setBraveThemeType(payload)
      break
    case types.IMPORT_BROWSER_DATA_REQUESTED:
      chrome.send('importData', [payload])
      break
    case types.GO_TO_TAB_REQUESTED:
      window.open(payload.url, payload.target)
      break
    case types.CLOSE_TAB_REQUESTED:
      window.close()
      break
    case types.CHANGE_DEFAULT_SEARCH_PROVIDER:
      const modelIndex = parseInt(payload, 10)
      chrome.send('setDefaultSearchEngine', [modelIndex])
      break
    case types.IMPORT_DEFAULT_SEARCH_PROVIDERS_SUCCESS:
      state = { ...state, searchProviders: payload }
      break
    case types.IMPORT_BROWSER_PROFILES_SUCCESS:
      state = { ...state, browserProfiles: payload }
      break
    case types.RECORD_P3A:
      let details = payload.details || {}
      chrome.send('recordP3A', [
        details.currentScreen,
        details.finished,
        details.skipped
      ])
      break
    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      state = { ...state, walletCreating: true }
      break
    case types.ON_WALLET_INITIALIZED: {
      const result: Welcome.WalletResult = payload.result
      state = { ...state }

      switch (result) {
        case Welcome.WalletResult.WALLET_CREATED:
          state.walletCreated = true
          state.walletCreating = false
          state.walletCreateFailed = false
          chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
          break
        case Welcome.WalletResult.WALLET_CORRUPT:
        case Welcome.WalletResult.LEDGER_OK:
          state.walletCreated = false
          state.walletCreating = false
          state.walletCreateFailed = true
          break
      }
      break
    }
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default welcomeReducer
