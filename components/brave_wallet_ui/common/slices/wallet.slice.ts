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
  WalletInitializedPayload,
  NetworkFilterType,
  RefreshOpts
} from '../../constants/types'
import {
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged
} from '../constants/action_types'

// Utils
import { parseJSONFromLocalStorage } from '../../utils/local-storage-utils'

// Options
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { AllAccountsOptionUniqueKey } from '../../options/account-filter-options'

const defaultState: WalletState = {
  hasInitialized: false,
  allowedNewWalletAccountTypeNetworkIds: [],
  isBitcoinEnabled: false,
  isZCashEnabled: false,
  isWalletCreated: false,
  isWalletLocked: true,
  addUserAssetError: false,
  activeOrigin: {
    eTldPlusOne: '',
    originSpec: ''
  },
  selectedNetworkFilter: parseJSONFromLocalStorage(
    'PORTFOLIO_NETWORK_FILTER_OPTION',
    AllNetworksOptionDefault
  ),
  selectedAccountFilter: AllAccountsOptionUniqueKey,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: true,
  isNftPinningFeatureEnabled: false,
  isAnkrBalancesFeatureEnabled: false,
  isRefreshingNetworksAndTokens: false
}

// async actions
export const WalletAsyncActions = {
  initialize: createAction<RefreshOpts>('initialize'),
  refreshAll: createAction<RefreshOpts>('refreshAll'),
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
  refreshNetworksAndTokens: createAction<RefreshOpts>(
    'refreshNetworksAndTokens'
  ),
  refreshBalancesAndPriceHistory: createAction(
    'refreshBalancesAndPriceHistory'
  ),
  setSelectedNetworkFilter: createAction<NetworkFilterType>(
    'setSelectedNetworkFilter'
  ),
  setSelectedAccountFilterItem: createAction<string>(
    'setSelectedAccountFilterItem'
  ),
  autoLockMinutesChanged: createAction('autoLockMinutesChanged') // No reducer or API logic for this (UNUSED)
}

// slice
export const createWalletSlice = (initialState: WalletState = defaultState) => {
  return createSlice({
    name: 'wallet',
    initialState,
    reducers: {
      activeOriginChanged(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.OriginInfo>
      ) {
        state.activeOrigin = payload
      },

      initialized(
        state: WalletState,
        { payload }: PayloadAction<WalletInitializedPayload>
      ) {
        state.hasInitialized = true
        state.isWalletCreated = payload.walletInfo.isWalletCreated
        state.isBitcoinEnabled = payload.walletInfo.isBitcoinEnabled
        state.isZCashEnabled = payload.walletInfo.isZCashEnabled
        state.isWalletLocked = payload.walletInfo.isWalletLocked
        state.isNftPinningFeatureEnabled =
          payload.walletInfo.isNftPinningFeatureEnabled
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

      builder.addCase(
        WalletAsyncActions.setSelectedAccountFilterItem,
        (state, { payload }) => {
          state.selectedAccountFilter = payload
        }
      )

      builder.addCase(
        WalletAsyncActions.setSelectedNetworkFilter,
        (state, { payload }) => {
          state.selectedNetworkFilter = payload
        }
      )
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
