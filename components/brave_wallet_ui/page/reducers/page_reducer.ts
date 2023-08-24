/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

import {
  BraveWallet,
  PageState,
  NFTMetadataReturnType
} from '../../constants/types'
import { getAssetIdKey } from '../../utils/asset-utils'
import {
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  ImportWalletErrorPayloadType,
  ShowRecoveryPhrasePayload,
  CreateWalletPayloadType,
  ImportAccountFromJsonPayloadType,
  ImportFromExternalWalletPayloadType,
  RestoreWalletPayloadType,
  UpdateSelectedAssetType,
  UpdateNftPinningStatusType
} from '../constants/action_types'

const defaultState: PageState = {
  hasInitialized: false,
  showRecoveryPhrase: false,
  invalidMnemonic: false,
  importWalletError: { hasError: false },
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  selectedAsset: undefined,
  isFetchingNFTMetadata: true,
  nftMetadata: undefined,
  nftMetadataError: undefined,
  enablingAutoPin: false,
  isAutoPinEnabled: false,
  pinStatusOverview: undefined,
  showIsRestoring: false,
  setupStillInProgress: false,
  isCryptoWalletsInitialized: false,
  isMetaMaskInitialized: false,
  isImportWalletsCheckComplete: false,
  importWalletAttempts: 0,
  walletTermsAcknowledged: false,
  selectedCoinMarket: undefined,
  nftsPinningStatus: {},
  isLocalIpfsNodeRunning: false
}

export const WalletPageAsyncActions = {
  addHardwareAccounts: createAction<BraveWallet.HardwareWalletAccount[]>('addHardwareAccounts'),
  checkWalletsToImport: createAction('checkWalletsToImport'),
  createWallet: createAction<CreateWalletPayloadType>('createWallet'),
  getNFTMetadata: createAction<BraveWallet.BlockchainToken>('getNFTMetadata'),
  importAccountFromJson: createAction<ImportAccountFromJsonPayloadType>('importAccountFromJson'),
  importFromCryptoWallets: createAction<ImportFromExternalWalletPayloadType>('importFromCryptoWallets'),
  importFromMetaMask: createAction<ImportFromExternalWalletPayloadType>('importFromMetaMask'),
  openWalletSettings: createAction('openWalletSettings'),
  restoreWallet: createAction<RestoreWalletPayloadType>('restoreWallet'),
  selectAsset: createAction<UpdateSelectedAssetType>('selectAsset'),
  updateNFTPinStatus: createAction<BraveWallet.TokenPinOverview | undefined>('updateNFTPinStatus'),
  getPinStatus: createAction<BraveWallet.BlockchainToken>('getPinStatus'),
  getIsAutoPinEnabled: createAction('getAutoPinEnabled'),
  setAutoPinEnabled: createAction<boolean>('setAutoPinEnabled'),
  updateEnablingAutoPin: createAction<boolean>('updateEnablingAutoPin'),
  updateAutoPinEnabled: createAction<boolean>('updateAutoPinEnabled'),
  getNftsPinningStatus: createAction<BraveWallet.BlockchainToken[]>('getNftsPinningStatus'),
  setNftsPinningStatus: createAction<UpdateNftPinningStatusType[]>('setNftsPinningStatus'),
  updateNftPinningStatus: createAction<UpdateNftPinningStatusType>('updateNftPinningStatus'),
  getLocalIpfsNodeStatus: createAction('getLocalIpfsNodeStatus'),
  updateLocalIpfsNodeStatus: createAction('updateLocalIpfsNodeStatus')
}

export const createPageSlice = (initialState: PageState = defaultState) => {
  return createSlice({
    name: 'page',
    initialState: initialState,
    reducers: {
      agreeToWalletTerms(state) {
        state.walletTermsAcknowledged = true
      },

      hasMnemonicError(state, { payload }: PayloadAction<boolean>) {
        state.invalidMnemonic = payload
      },

      recoveryWordsAvailable(
        state,
        { payload }: PayloadAction<RecoveryWordsAvailablePayloadType>
      ) {
        if (state.mnemonic !== payload.mnemonic) {
          state.mnemonic = payload.mnemonic
        }
      },

      selectCoinMarket(
        state,
        { payload }: PayloadAction<BraveWallet.CoinMarket | undefined>
      ) {
        state.selectedCoinMarket = payload
      },

      setCryptoWalletsInitialized(state, { payload }: PayloadAction<boolean>) {
        state.isCryptoWalletsInitialized = payload
      },

      setImportWalletError(
        state,
        { payload }: PayloadAction<ImportWalletErrorPayloadType>
      ) {
        const { hasError, errorMessage, incrementAttempts } = payload

        state.importWalletError = { hasError, errorMessage }

        if (incrementAttempts) {
          state.importWalletAttempts = state.importWalletAttempts + 1
        }
      },

      setImportWalletsCheckComplete(
        state,
        { payload }: PayloadAction<boolean>
      ) {
        state.isImportWalletsCheckComplete = payload
      },

      setIsFetchingNFTMetadata(state, { payload }: PayloadAction<boolean>) {
        state.isFetchingNFTMetadata = payload
      },

      setMetaMaskInitialized(state, { payload }: PayloadAction<boolean>) {
        state.isMetaMaskInitialized = payload
      },

      setShowIsRestoring(state, action: PayloadAction<boolean>) {
        state.showIsRestoring = action.payload
      },

      showRecoveryPhrase(
        state,
        { payload }: PayloadAction<ShowRecoveryPhrasePayload>
      ) {
        state.showRecoveryPhrase = payload.show
      },

      updateNFTMetadata(
        state,
        { payload }: PayloadAction<NFTMetadataReturnType | undefined>
      ) {
        state.nftMetadata = payload
      },

      updateNftMetadataError(
        state,
        { payload }: PayloadAction<string | undefined>
      ) {
        state.nftMetadataError = payload
      },

      selectPriceTimeframe(
        state,
        { payload }: PayloadAction<BraveWallet.AssetPriceTimeframe>
      ) {
        state.selectedTimeline = payload
      },

      updateSelectedAsset(
        state,
        { payload }: PayloadAction<BraveWallet.BlockchainToken | undefined>
      ) {
        state.selectedAsset = payload
      },

      walletBackupComplete(state: PageState) {
        state.showRecoveryPhrase = false
        state.mnemonic = undefined
      },

      walletCreated(
        state,
        { payload }: PayloadAction<WalletCreatedPayloadType>
      ) {
        state.mnemonic = payload.mnemonic
        state.setupStillInProgress = true
      },

      walletSetupComplete(state, action?: PayloadAction<boolean>) {
        // complete setup unless explicitly halted
        state.setupStillInProgress = !action?.payload
        state.mnemonic = undefined
      },

      updateNFTPinStatus(
        state,
        { payload }: PayloadAction<BraveWallet.TokenPinOverview | undefined>
      ) {
        state.pinStatusOverview = payload
      },

      updateEnablingAutoPin(state, { payload }: PayloadAction<boolean>) {
        state.enablingAutoPin = payload
      },

      updateAutoPinEnabled(state, { payload }: PayloadAction<boolean>) {
        state.isAutoPinEnabled = payload
      },

      setNftsPinningStatus(
        state,
        { payload }: PayloadAction<UpdateNftPinningStatusType[]>
      ) {
        const pinningStatus = {}
        payload.forEach(({ token, status }) => {
          const { code, error } = status || {}
          pinningStatus[getAssetIdKey(token)] = { code, error }
        })

        state.nftsPinningStatus = {
          ...state.nftsPinningStatus,
          ...pinningStatus
        }
      },

      updateNftPinningStatus(
        state,
        { payload }: PayloadAction<UpdateNftPinningStatusType>
      ) {
        const { token, status } = payload
        const { code, error } = status || {}
        state.nftsPinningStatus = {
          ...state.nftsPinningStatus,
          [getAssetIdKey(token)]: { code, error }
        }
      },

      updateLocalIpfsNodeStatus(state, { payload }: PayloadAction<boolean>) {
        state.isLocalIpfsNodeRunning = payload
      }
    }
  })
}

export const createPageReducer = (initialState: PageState) => {
  return createPageSlice(initialState).reducer
}

export const pageSlice = createPageSlice()
export const pageReducer = pageSlice.reducer
export const PageActions = { ...WalletPageAsyncActions, ...pageSlice.actions }
export default pageReducer
