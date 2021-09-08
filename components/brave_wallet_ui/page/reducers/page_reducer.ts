/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import * as Actions from '../actions/wallet_page_actions'
import {
  PageState,
  AssetPriceTimeframe,
  TokenInfo
} from '../../constants/types'
import {
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  PrivateKeyAvailablePayloadType,
  SelectAssetPayloadType
} from '../constants/action_types'

const defaultState: PageState = {
  hasInitialized: false,
  showAddModal: false,
  showRecoveryPhrase: false,
  invalidMnemonic: false,
  importError: false,
  selectedTimeline: AssetPriceTimeframe.OneDay,
  selectedAsset: undefined,
  selectedUSDAssetPrice: undefined,
  selectedBTCAssetPrice: undefined,
  selectedAssetPriceHistory: [],
  portfolioPriceHistory: [],
  isFetchingPriceHistory: false,
  showIsRestoring: false,
  setupStillInProgress: false
}

const reducer = createReducer<PageState>({}, defaultState)

reducer.on(Actions.walletCreated, (state: PageState, payload: WalletCreatedPayloadType) => {
  return {
    ...state,
    mnemonic: payload.mnemonic,
    setupStillInProgress: true
  }
})

reducer.on(Actions.recoveryWordsAvailable, (state: PageState, payload: RecoveryWordsAvailablePayloadType) => {
  return {
    ...state,
    mnemonic: payload.mnemonic
  }
})

reducer.on(Actions.privateKeyAvailable, (state: PageState, payload: PrivateKeyAvailablePayloadType) => {
  return {
    ...state,
    privateKey: payload.privateKey
  }
})

reducer.on(Actions.doneViewingPrivateKey, (state: PageState) => {
  const newState = { ...state }
  delete newState.privateKey
  return newState
})

reducer.on(Actions.walletSetupComplete, (state: PageState) => {
  const newState = { ...state }
  delete newState.mnemonic
  delete newState.setupStillInProgress
  return newState
})

reducer.on(Actions.walletBackupComplete, (state: PageState) => {
  const newState = {
    ...state,
    showRecoveryPhrase: false
  }
  delete newState.mnemonic
  return newState
})

reducer.on(Actions.showRecoveryPhrase, (state: PageState, payload: boolean) => {
  return {
    ...state,
    showRecoveryPhrase: payload
  }
})

reducer.on(Actions.hasMnemonicError, (state: PageState, payload: boolean) => {
  return {
    ...state,
    invalidMnemonic: payload
  }
})

reducer.on(Actions.updateSelectedAsset, (state: PageState, payload: TokenInfo) => {
  return {
    ...state,
    selectedAsset: payload
  }
})

reducer.on(Actions.updatePriceInfo, (state: PageState, payload: SelectAssetPayloadType) => {
  const history = payload.priceHistory ? payload.priceHistory.values : []
  return {
    ...state,
    selectedUSDAssetPrice: payload.usdPriceInfo,
    selectedBTCAssetPrice: payload.btcPriceInfo,
    selectedAssetPriceHistory: history,
    selectedTimeline: payload.timeFrame,
    isFetchingPriceHistory: false
  }
})

reducer.on(Actions.setIsFetchingPriceHistory, (state: PageState, payload: boolean) => {
  return {
    ...state,
    isFetchingPriceHistory: payload
  }
})

reducer.on(Actions.setShowIsRestoring, (state: PageState, payload: boolean) => {
  return {
    ...state,
    showIsRestoring: payload
  }
})

reducer.on(Actions.setImportError, (state: PageState, payload: boolean) => {
  return {
    ...state,
    importError: payload
  }
})

reducer.on(Actions.setShowAddModal, (state: PageState, payload: boolean) => {
  return {
    ...state,
    showAddModal: payload
  }
})

export default reducer
