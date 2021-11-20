/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/gemini_types'
import { GeminiAssetAddress } from '../actions//gemini_actions'
import * as storage from '../storage/new_tab_storage'

const geminiReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_VALID_GEMINI_AUTH_CODE:
      state = { ...state }
      state.geminiState.userAuthed = true
      state.geminiState.authInProgress = false
      break

    case types.CONNECT_TO_GEMINI:
      state = { ...state }
      state.geminiState.authInProgress = true
      break

    case types.ON_GEMINI_CLIENT_URL:
      const { clientUrl } = payload
      state = { ...state }
      state.geminiState.geminiClientUrl = clientUrl
      break

    case types.SET_GEMINI_TICKER_PRICE:
      const { asset, price } = payload
      state = { ...state }
      state.geminiState.tickerPrices[asset] = price
      break

    case types.SET_SELECTED_VIEW:
      const { view } = payload
      state = { ...state }
      state.geminiState = {
        ...state.geminiState,
        selectedView: view
      }
      break

    case types.SET_HIDE_BALANCE:
      state = { ...state }
      state.geminiState.hideBalance = payload.hide
      break

    case types.SET_ACCOUNT_BALANCES:
      const { balances } = payload
      state = { ...state }
      for (let balance in balances) {
        if (balances[balance]) {
          state.geminiState.accountBalances[balance] = balances[balance]
        }
      }
      break

    case types.SET_ASSET_ADDRESS:
      state = { ...state }
      payload.assetAddresses.forEach((assetAddress: GeminiAssetAddress) => {
        state.geminiState.assetAddresses[assetAddress.asset] = assetAddress.address
        state.geminiState.assetAddressQRCodes[assetAddress.asset] = assetAddress.qrCode
      })
      break

    case types.DISCONNECT_GEMINI:
      state = { ...state }
      state.geminiState = {
        ...storage.defaultState.geminiState
      }
      break

    case types.SET_DISCONNECT_IN_PROGRESS:
      const { inProgress } = payload
      state = { ...state }
      state.geminiState = {
        ...state.geminiState,
        disconnectInProgress: inProgress
      }
      break

    case types.SET_AUTH_INVALID:
      const { authInvalid } = payload
      state = { ...state }
      state.geminiState = {
        ...state.geminiState,
        authInvalid
      }
      break

    default:
      break
  }

  return state
}

export default geminiReducer
