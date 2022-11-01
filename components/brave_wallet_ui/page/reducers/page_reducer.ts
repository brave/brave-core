/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

import * as Actions from '../actions/wallet_page_actions'
import {
  BraveWallet,
  PageState,
  NFTMetadataReturnType,
  ImportAccountErrorType,
  UpdateAccountNamePayloadType
} from '../../constants/types'
import {
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  SelectAssetPayloadType,
  ImportWalletErrorPayloadType,
  ShowRecoveryPhrasePayload,
  CreateWalletPayloadType,
  ImportAccountFromJsonPayloadType,
  ImportAccountPayloadType,
  ImportFilecoinAccountPayloadType,
  ImportFromExternalWalletPayloadType,
  RemoveHardwareAccountPayloadType,
  RemoveImportedAccountPayloadType,
  RestoreWalletPayloadType,
  UpdateSelectedAssetType
} from '../constants/action_types'

const defaultState: PageState = {
  hasInitialized: false,
  showAddModal: false,
  showRecoveryPhrase: false,
  invalidMnemonic: false,
  importAccountError: undefined,
  importWalletError: { hasError: false },
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  selectedAsset: undefined,
  isFetchingNFTMetadata: true,
  nftMetadata: undefined,
  nftMetadataError: undefined,
  selectedAssetFiatPrice: undefined,
  selectedAssetCryptoPrice: undefined,
  selectedAssetPriceHistory: [],
  portfolioPriceHistory: [],
  isFetchingPriceHistory: false,
  showIsRestoring: false,
  setupStillInProgress: false,
  isCryptoWalletsInitialized: false,
  isMetaMaskInitialized: false,
  isImportWalletsCheckComplete: false,
  importWalletAttempts: 0,
  walletTermsAcknowledged: false,
  selectedCoinMarket: undefined
}

export const WalletPageAsyncActions = {
  createWallet: createAction<CreateWalletPayloadType>('createWallet'),
  restoreWallet: createAction<RestoreWalletPayloadType>('restoreWallet'),
  importAccount: createAction<ImportAccountPayloadType>('importAccount'),
  importFilecoinAccount: createAction<ImportFilecoinAccountPayloadType>('importFilecoinAccount'),
  importAccountFromJson: createAction<ImportAccountFromJsonPayloadType>('importAccountFromJson'),
  removeImportedAccount: createAction<RemoveImportedAccountPayloadType>('removeImportedAccount'),
  walletCreated: createAction<WalletCreatedPayloadType>('walletCreated'),
  walletSetupComplete: createAction<boolean>('walletSetupComplete'),
  showRecoveryPhrase: createAction<ShowRecoveryPhrasePayload>('showRecoveryPhrase'),
  recoveryWordsAvailable: createAction<RecoveryWordsAvailablePayloadType>('recoveryWordsAvailable'),
  walletBackupComplete: createAction('walletBackupComplete'),
  hasMnemonicError: createAction<boolean>('hasMnemonicError'),
  setShowAddModal: createAction<boolean>('setShowAddModal'),
  setImportAccountError: createAction<ImportAccountErrorType>('setImportAccountError'),
  setImportWalletError: createAction<ImportWalletErrorPayloadType>('setImportWalletError'),
  updatePriceInfo: createAction<SelectAssetPayloadType>('updatePriceInfo'),
  selectAsset: createAction<UpdateSelectedAssetType>('selectAsset'),
  updateSelectedAsset: createAction<BraveWallet.BlockchainToken | undefined>('updateSelectedAsset'),
  setIsFetchingPriceHistory: createAction<boolean>('setIsFetchingPriceHistory'),
  setShowIsRestoring: createAction<boolean>('setShowIsRestoring'),
  updateAccountName: createAction<UpdateAccountNamePayloadType>('updateAccountName'),
  addHardwareAccounts: createAction<BraveWallet.HardwareWalletAccount[]>('addHardwareAccounts'),
  removeHardwareAccount: createAction<RemoveHardwareAccountPayloadType>('removeHardwareAccount'),
  checkWalletsToImport: createAction('checkWalletsToImport'),
  setCryptoWalletsInitialized: createAction<boolean>('setCryptoWalletsInitialized'),
  setMetaMaskInitialized: createAction<boolean>('setMetaMaskInitialized'),
  setImportWalletsCheckComplete: createAction<boolean>('setImportWalletsCheckComplete'),
  importFromCryptoWallets: createAction<ImportFromExternalWalletPayloadType>('importFromCryptoWallets'),
  importFromMetaMask: createAction<ImportFromExternalWalletPayloadType>('importFromMetaMask'),
  openWalletSettings: createAction('openWalletSettings'),
  getNFTMetadata: createAction<BraveWallet.BlockchainToken>('getNFTMetadata'),
  setIsFetchingNFTMetadata: createAction<boolean>('setIsFetchingNFTMetadata'),
  updateNFTMetadata: createAction<NFTMetadataReturnType | undefined>('updateNFTMetadata'),
  onOnboardingShown: createAction('onOnboardingShown'),
  agreeToWalletTerms: createAction('agreeToWalletTerms'),
  selectCoinMarket: createAction<BraveWallet.CoinMarket | undefined>('selectCoinMarket')
}

export const createPageSlice = (initialState: PageState = defaultState) => {
  return createSlice({
    name: 'page',
    initialState: initialState,
    reducers: {
      hasMnemonicError (state: PageState, { payload }: PayloadAction<boolean>) {
        state.invalidMnemonic = payload
      },

      recoveryWordsAvailable (state: PageState, { payload }: PayloadAction<RecoveryWordsAvailablePayloadType>) {
        state.mnemonic = payload.mnemonic
      },

      setIsFetchingPriceHistory (state: PageState, { payload }: PayloadAction<boolean>) {
        state.isFetchingPriceHistory = payload
      },

      showRecoveryPhrase (state: PageState, { payload }: PayloadAction<ShowRecoveryPhrasePayload>) {
        state.showRecoveryPhrase = payload.show
      },

      updatePriceInfo (state: PageState, { payload }: PayloadAction<SelectAssetPayloadType>) {
        state.selectedAssetFiatPrice = payload.defaultFiatPrice
        state.selectedAssetCryptoPrice = payload.defaultCryptoPrice
        state.selectedAssetPriceHistory = payload.priceHistory?.values || []
        state.selectedTimeline = payload.timeFrame
        state.isFetchingPriceHistory = false
      },

      updateSelectedAsset (state: PageState, { payload }: PayloadAction<BraveWallet.BlockchainToken | undefined>) {
        state.selectedAsset = payload
      },

      walletBackupComplete (state: PageState) {
        state.showRecoveryPhrase = false
        state.mnemonic = undefined
      },

      walletCreated (state, { payload }: PayloadAction<WalletCreatedPayloadType>) {
        state.mnemonic = payload.mnemonic
        state.setupStillInProgress = true
      },

      walletSetupComplete (state: PageState, action?: PayloadAction<boolean>) {
        // complete setup unless explicitly halted
        state.setupStillInProgress = !action?.payload
        state.mnemonic = undefined
      }
    }
  })
}

export const createPageReducer = (initialState: PageState) => {
  const reducer = createReducer<PageState>({}, initialState)

  reducer.on(Actions.setShowIsRestoring.type, (state: PageState, payload: boolean) => {
    return {
      ...state,
      showIsRestoring: payload
    }
  })

  reducer.on(Actions.setImportAccountError.type, (state: PageState, payload: ImportAccountErrorType) => {
    return {
      ...state,
      importAccountError: payload
    }
  })

  reducer.on(Actions.setImportWalletError.type, (state: PageState, {
    hasError,
    errorMessage,
    incrementAttempts
  }: ImportWalletErrorPayloadType) => {
    return {
      ...state,
      importWalletError: { hasError, errorMessage },
      importWalletAttempts: incrementAttempts ? state.importWalletAttempts + 1 : state.importWalletAttempts
    }
  })

  reducer.on(Actions.setShowAddModal.type, (state: PageState, payload: boolean) => {
    return {
      ...state,
      showAddModal: payload
    }
  })

  reducer.on(Actions.setCryptoWalletsInitialized.type, (state: PageState, payload: boolean): PageState => {
    return {
      ...state,
      isCryptoWalletsInitialized: payload
    }
  })

  reducer.on(Actions.setMetaMaskInitialized.type, (state: PageState, payload: boolean) => {
    return {
      ...state,
      isMetaMaskInitialized: payload
    }
  })

  reducer.on(Actions.updateNFTMetadata.type, (state: PageState, payload: NFTMetadataReturnType) => {
    return {
      ...state,
      nftMetadata: payload
    }
  })

  reducer.on(Actions.setIsFetchingNFTMetadata.type, (state: PageState, payload: boolean) => {
    return {
      ...state,
      isFetchingNFTMetadata: payload
    }
  })

  reducer.on(Actions.updateNftMetadataError.type, (state: PageState, payload: string | undefined) => {
    return {
      ...state,
      nftMetadataError: payload
    }
  })

  reducer.on(Actions.setImportWalletsCheckComplete.type, (state: PageState, payload: boolean): PageState => {
    return {
      ...state,
      isImportWalletsCheckComplete: payload
    }
  })

  reducer.on(Actions.agreeToWalletTerms.type, (state: PageState): PageState => {
    return {
      ...state,
      walletTermsAcknowledged: true
    }
  })

  reducer.on(Actions.selectCoinMarket.type, (state: PageState, payload: BraveWallet.CoinMarket) => {
    return {
      ...state,
      selectedCoinMarket: payload
    }
  })
  return reducer
}

const reducer = createPageReducer(defaultState)
export const pageReducer = reducer
export default reducer
