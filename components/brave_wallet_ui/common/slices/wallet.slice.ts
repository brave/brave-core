/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  createAction,
  createSlice,
  PayloadAction,
  EntityId
} from '@reduxjs/toolkit'

import {
  BraveWallet,
  WalletState,
  WalletInitializedPayload
} from '../../constants/types'
import {
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged
} from '../constants/action_types'

const defaultState: WalletState = {
  hasInitialized: false,
  allowedNewWalletAccountTypeNetworkIds: [],
  isBitcoinEnabled: false,
  isBitcoinImportEnabled: false,
  isBitcoinLedgerEnabled: false,
  isZCashEnabled: false,
  isWalletCreated: false,
  isWalletLocked: true,
  addUserAssetError: false,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: true,
  isAnkrBalancesFeatureEnabled: false,
  isRefreshingNetworksAndTokens: false
}

// async actions
export const WalletAsyncActions = {
  initialize: createAction('initialize'),
  refreshAll: createAction('refreshAll'),
  selectAccount: createAction<BraveWallet.AccountId>('selectAccount'), // should use apiProxy - keyringService
  getAllNetworks: createAction('getAllNetworks'), // alias to refreshFullNetworkList
  walletCreated: createAction('walletCreated'),
  walletRestored: createAction('walletRestored'),
  walletReset: createAction('walletReset'),
  locked: createAction('locked'),
  unlocked: createAction('unlocked'),
  backedUp: createAction('backedUp'),
  defaultBaseCurrencyChanged: createAction<DefaultBaseCurrencyChanged>(
    'defaultBaseCurrencyChanged'
  ), // refreshWalletInfo
  defaultBaseCryptocurrencyChanged:
    createAction<DefaultBaseCryptocurrencyChanged>(
      'defaultBaseCryptocurrencyChanged'
    ),
  refreshNetworksAndTokens: createAction('refreshNetworksAndTokens'),
  refreshBalancesAndPriceHistory: createAction(
    'refreshBalancesAndPriceHistory'
  ),
  autoLockMinutesChanged: createAction('autoLockMinutesChanged') // No reducer or API logic for this (UNUSED)
}

// slice
export const createWalletSlice = (initialState: WalletState = defaultState) => {
  return createSlice({
    name: 'wallet',
    initialState,
    reducers: {
      initialized(
        state: WalletState,
        { payload }: PayloadAction<WalletInitializedPayload>
      ) {
        state.hasInitialized = true
        state.isWalletCreated = payload.walletInfo.isWalletCreated
        state.isBitcoinEnabled = payload.walletInfo.isBitcoinEnabled
        state.isBitcoinImportEnabled = payload.walletInfo.isBitcoinImportEnabled
        state.isBitcoinLedgerEnabled = payload.walletInfo.isBitcoinLedgerEnabled
        state.isZCashEnabled = payload.walletInfo.isZCashEnabled
        state.isWalletLocked = payload.walletInfo.isWalletLocked
        state.isAnkrBalancesFeatureEnabled =
          payload.walletInfo.isAnkrBalancesFeatureEnabled
      },

      setAssetAutoDiscoveryCompleted(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.assetAutoDiscoveryCompleted = payload
      },

      setPasswordAttempts(
        state: WalletState,
        { payload }: PayloadAction<number>
      ) {
        state.passwordAttempts = payload
      },

      setIsRefreshingNetworksAndTokens: (
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) => {
        state.isRefreshingNetworksAndTokens = payload
      },

      setAllowedNewWalletAccountTypeNetworkIds(
        state: WalletState,
        { payload }: PayloadAction<EntityId[]>
      ) {
        state.allowedNewWalletAccountTypeNetworkIds = payload
      }
    },
    extraReducers: (builder) => {
      builder.addCase(WalletAsyncActions.locked.type, (state) => {
        state.isWalletLocked = true
      })
    }
  })
}

export const createWalletReducer = (initialState: WalletState) => {
  return createWalletSlice(initialState).reducer
}

export const walletSlice = createWalletSlice()
export const walletReducer = walletSlice.reducer
export const WalletActions = { ...walletSlice.actions, ...WalletAsyncActions }
export default walletReducer
