/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'
import { types } from '../constants/binance_types'
import { isValidClientURL } from '../binance-utils'
import * as storage from '../storage/new_tab_storage'

const binanceReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State, action) => {
  const payload = action.payload

  switch (action.type) {
    case types.ON_BINANCE_USER_TLD:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          userTLD: payload.userTLD
        }
      }
      break

    case types.ON_BINANCE_USER_LOCALE:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          userLocale: payload.userLocale
        }
      }
      break

    case types.SET_INITIAL_AMOUNT:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialAmount: payload.initialAmount
        }
      }
      break

    case types.SET_INITIAL_FIAT:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialFiat: payload.initialFiat
        }
      }
      break

    case types.SET_INITIAL_ASSET:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          initialAsset: payload.initialAsset
        }
      }
      break

    case types.SET_USER_TLD_AUTO_SET:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          userTLDAutoSet: true
        }
      }
      break

    case types.SET_BINANCE_SUPPORTED:
      state = { ...state }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          binanceSupported: payload.supported
        }
      }
      break

    case types.SET_HIDE_BALANCE:
      state = { ...state }
      state.binanceState.hideBalance = payload.hide
      break

    case types.ON_VALID_BINANCE_AUTH_CODE:
      state = { ...state }
      state.binanceState.userAuthed = true
      state.binanceState.authInProgress = false
      break

    case types.DISCONNECT_BINANCE:
      state = { ...state }
      state.binanceState = {
        ...storage.defaultState.binanceState
      }
      break

    case types.CONNECT_TO_BINANCE:
      state = { ...state }
      state.binanceState.authInProgress = true
      break

    case types.ON_BINANCE_CLIENT_URL:
      const { clientUrl } = payload

      if (!isValidClientURL(clientUrl)) {
        break
      }

      state = { ...state }
      state.binanceState.binanceClientUrl = clientUrl
      break

    case types.ON_ASSETS_BALANCE_INFO:
      state = { ...state }
      const balances = payload.info

      if (!state.binanceState.accountBalances) {
        state.binanceState.accountBalances = {}
      }

      if (!state.binanceState.assetUSDValues) {
        state.binanceState.assetUSDValues = {}
      }

      let totalBtcValue = 0.00
      let totalUSDValue = 0.00

      for (let ticker in balances) {
        const balance = balances[ticker].balance
        const usdValue = balances[ticker].fiatValue
        const btcValue = balances[ticker].btcValue
        const assetUSDValue = parseFloat(usdValue).toFixed(2)

        state.binanceState.accountBalances[ticker] = balance
        state.binanceState.assetUSDValues[ticker] = assetUSDValue.toString()

        totalUSDValue += parseFloat(usdValue)
        totalBtcValue += parseFloat(btcValue)
      }

      const usdValue = totalUSDValue.toFixed(2).toString()
      state.binanceState.accountBTCUSDValue = usdValue
      state.binanceState.accountBTCValue = totalBtcValue.toString()
      break

    case types.ON_ASSET_DEPOSIT_INFO:
      const { symbol, address, tag } = payload
      if (!symbol || (!address && !tag)) {
        break
      }

      state = { ...state }
      if (!state.binanceState.assetDepositInfo) {
        state.binanceState.assetDepositInfo = {}
      }
      state.binanceState.assetDepositInfo[symbol] = {
        address,
        tag
      }
      break

    case types.ON_DEPOSIT_QR_FOR_ASSET:
      const { asset, imageSrc } = payload
      if (!asset || !imageSrc) {
        break
      }

      state = { ...state }
      if (!state.binanceState.assetDepoitQRCodeSrcs) {
        state.binanceState.assetDepoitQRCodeSrcs = {}
      }
      state.binanceState.assetDepoitQRCodeSrcs[asset] = imageSrc
      break

    case types.ON_CONVERTABLE_ASSETS:
      type Payload = {
         assets: chrome.binance.ConvertAssets
      }
      const { assets }: Payload = payload
      if (!assets) {
        break
      }
      state = {
        ...state,
        binanceState: {
          ...state.binanceState,
          convertAssets: assets
        }
      }
      break

    case types.SET_DISCONNECT_IN_PROGRESS:
      const { inProgress } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        disconnectInProgress: inProgress
      }
      break

    case types.SET_AUTH_INVALID:
      const { authInvalid } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        authInvalid
      }
      break

    case types.SET_SELECTED_VIEW:
      const { view } = payload
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        selectedView: view
      }
      break

    case types.SET_DEPOSIT_INFO_SAVED:
      state = { ...state }
      state.binanceState = {
        ...state.binanceState,
        depositInfoSaved: true
      }
      break

    default:
      break
  }

  return state
}

export default binanceReducer
