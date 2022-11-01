/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

import {
  BraveWallet,
  PageState,
  NFTMetadataReturnType,
  ImportAccountErrorType
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
  selectAsset: createAction<UpdateSelectedAssetType>('selectAsset'),
  addHardwareAccounts: createAction<BraveWallet.HardwareWalletAccount[]>('addHardwareAccounts'),
  removeHardwareAccount: createAction<RemoveHardwareAccountPayloadType>('removeHardwareAccount'),
  checkWalletsToImport: createAction('checkWalletsToImport'),
  importFromCryptoWallets: createAction<ImportFromExternalWalletPayloadType>('importFromCryptoWallets'),
  importFromMetaMask: createAction<ImportFromExternalWalletPayloadType>('importFromMetaMask'),
  openWalletSettings: createAction('openWalletSettings'),
  getNFTMetadata: createAction<BraveWallet.BlockchainToken>('getNFTMetadata')
}

export const createPageSlice = (initialState: PageState = defaultState) => {
  return createSlice({
    name: 'page',
    initialState: initialState,
    reducers: {
      agreeToWalletTerms (state) {
        state.walletTermsAcknowledged = true
      },

      hasMnemonicError (state: PageState, { payload }: PayloadAction<boolean>) {
        state.invalidMnemonic = payload
      },

      recoveryWordsAvailable (state: PageState, { payload }: PayloadAction<RecoveryWordsAvailablePayloadType>) {
        state.mnemonic = payload.mnemonic
      },

      selectCoinMarket (state, { payload }: PayloadAction<BraveWallet.CoinMarket>) {
        state.selectedCoinMarket = payload
      },

      setCryptoWalletsInitialized (state, { payload }: PayloadAction<boolean>) {
        state.isCryptoWalletsInitialized = payload
      },

      setImportAccountError (state: PageState, { payload }: PayloadAction<ImportAccountErrorType>) {
        state.importAccountError = payload
      },

      setImportWalletError (state, { payload }: PayloadAction<ImportWalletErrorPayloadType>) {
        const { hasError, errorMessage, incrementAttempts } = payload

        state.importWalletError = { hasError, errorMessage }

        if (incrementAttempts) {
          state.importWalletAttempts = state.importWalletAttempts + 1
        }
      },

      setImportWalletsCheckComplete (state, { payload }: PayloadAction<boolean>) {
        state.isImportWalletsCheckComplete = payload
      },

      setIsFetchingNFTMetadata (state, { payload }: PayloadAction<boolean>) {
        state.isFetchingNFTMetadata = payload
      },

      setIsFetchingPriceHistory (state: PageState, { payload }: PayloadAction<boolean>) {
        state.isFetchingPriceHistory = payload
      },

      setMetaMaskInitialized (state, { payload }: PayloadAction<boolean>) {
        state.isMetaMaskInitialized = payload
      },

      setShowAddModal (state, { payload }: PayloadAction<boolean>) {
        state.showAddModal = payload
      },

      setShowIsRestoring (state: PageState, action: PayloadAction<boolean>) {
        state.showIsRestoring = action.payload
      },

      showRecoveryPhrase (state: PageState, { payload }: PayloadAction<ShowRecoveryPhrasePayload>) {
        state.showRecoveryPhrase = payload.show
      },

      updateNFTMetadata (state, { payload }: PayloadAction<NFTMetadataReturnType>) {
        state.nftMetadata = payload
      },

      updateNftMetadataError (state, { payload }: PayloadAction<string | undefined>) {
        state.nftMetadataError = payload
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
  const { reducer } = createPageSlice(initialState)
  return reducer
}

export const pageSlice = createPageSlice()
export const pageReducer = pageSlice.reducer
export const PageActions = { ...pageSlice.actions, ...WalletPageAsyncActions }
export default pageReducer
