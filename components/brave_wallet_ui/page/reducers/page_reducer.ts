/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import * as Actions from '../actions/wallet_page_actions'
import {
  BraveWallet,
  PageState,
  ImportWalletError
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
  importAccountError: false,
  importWalletError: { hasError: false },
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  selectedAsset: undefined,
  selectedAssetFiatPrice: undefined,
  selectedAssetCryptoPrice: undefined,
  selectedAssetPriceHistory: [],
  portfolioPriceHistory: [],
  isFetchingPriceHistory: false,
  showIsRestoring: false,
  setupStillInProgress: false,
  isCryptoWalletsInitialized: false,
  isMetaMaskInitialized: false
}

export const createPageReducer = (initialState: PageState) => {
  const reducer = createReducer<PageState>({}, initialState)
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
    const newState: PageState = {
      ...state,
      setupStillInProgress: false
    }
    delete newState.mnemonic
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

  reducer.on(Actions.updateSelectedAsset, (state: PageState, payload: BraveWallet.BlockchainToken) => {
    return {
      ...state,
      selectedAsset: payload
    }
  })

  reducer.on(Actions.updatePriceInfo, (state: PageState, payload: SelectAssetPayloadType) => {
    const history = payload.priceHistory ? payload.priceHistory.values : []
    return {
      ...state,
      selectedAssetFiatPrice: payload.defaultFiatPrice,
      selectedAssetCryptoPrice: payload.defaultCryptoPrice,
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

  reducer.on(Actions.setImportAccountError, (state: PageState, payload: boolean) => {
    return {
      ...state,
      importAccountError: payload
    }
  })

  reducer.on(Actions.setImportWalletError, (state: PageState, payload: ImportWalletError) => {
    return {
      ...state,
      importWalletError: payload
    }
  })

  reducer.on(Actions.setShowAddModal, (state: PageState, payload: boolean) => {
    return {
      ...state,
      showAddModal: payload
    }
  })

  reducer.on(Actions.setCryptoWalletsInitialized, (state: PageState, payload: boolean) => {
    return {
      ...state,
      isCryptoWalletsInstalled: payload
    }
  })

  reducer.on(Actions.setMetaMaskInitialized, (state: PageState, payload: boolean) => {
    return {
      ...state,
      isMetaMaskInitialized: payload
    }
  })
  return reducer
}

const reducer = createPageReducer(defaultState)
export const pageReducer = reducer
export default reducer
