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
import {
  WalletCreatedPayloadType,
  RecoveryWordsAvailablePayloadType,
  ImportAccountFromJsonPayloadType,
  UpdateSelectedAssetType
} from '../constants/action_types'

const defaultState: PageState = {
  hasInitialized: false,
  showRecoveryPhrase: false,
  selectedTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
  selectedAsset: undefined,
  isFetchingNFTMetadata: true,
  nftMetadata: undefined,
  nftMetadataError: undefined,
  enablingAutoPin: false,
  isAutoPinEnabled: false,
  pinStatusOverview: undefined,
  setupStillInProgress: false,
  walletTermsAcknowledged: false,
  selectedCoinMarket: undefined
}

export const WalletPageAsyncActions = {
  addHardwareAccounts: createAction<BraveWallet.HardwareWalletAccount[]>(
    'addHardwareAccounts'
  ),
  importAccountFromJson: createAction<ImportAccountFromJsonPayloadType>(
    'importAccountFromJson'
  ),
  openWalletSettings: createAction('openWalletSettings'),
  selectAsset: createAction<UpdateSelectedAssetType>('selectAsset')
}

export const createPageSlice = (initialState: PageState = defaultState) => {
  return createSlice({
    name: 'page',
    initialState: initialState,
    reducers: {
      agreeToWalletTerms(state) {
        state.walletTermsAcknowledged = true
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

      setIsFetchingNFTMetadata(state, { payload }: PayloadAction<boolean>) {
        state.isFetchingNFTMetadata = payload
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
