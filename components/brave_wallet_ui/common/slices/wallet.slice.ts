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
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

// Utils
import {
  parseJSONFromLocalStorage,
  makeInitialFilteredOutNetworkKeys
} from '../../utils/local-storage-utils'

// Options
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import { NoneGroupByOption } from '../../options/group-assets-by-options'
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
  selectedAssetFilter:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION
    ) || HighToLowAssetsFilterOption.id,
  selectedGroupAssetsByItem:
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY) ||
    NoneGroupByOption.id,
  selectedAccountFilter: AllAccountsOptionUniqueKey,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: true,
  isNftPinningFeatureEnabled: false,
  isAnkrBalancesFeatureEnabled: false,
  hidePortfolioGraph:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN
    ) === 'true',
  hidePortfolioBalances:
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_BALANCES) ===
    'true',
  hidePortfolioNFTsTab:
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB) ===
    'true',
  filteredOutPortfolioNetworkKeys: parseJSONFromLocalStorage(
    'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS',
    makeInitialFilteredOutNetworkKeys()
  ),
  filteredOutPortfolioAccountIds: parseJSONFromLocalStorage(
    'FILTERED_OUT_PORTFOLIO_ACCOUNT_IDS',
    []
  ),
  hidePortfolioSmallBalances:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_SMALL_BALANCES
    ) === 'true',
  showNetworkLogoOnNfts:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.SHOW_NETWORK_LOGO_ON_NFTS
    ) === 'true',
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

      setSelectedAssetFilterItem(
        state: WalletState,
        { payload }: PayloadAction<string>
      ) {
        state.selectedAssetFilter = payload
      },

      setSelectedGroupAssetsByItem(
        state: WalletState,
        { payload }: PayloadAction<string>
      ) {
        state.selectedGroupAssetsByItem = payload
      },

      setHidePortfolioGraph(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.hidePortfolioGraph = payload
      },

      setFilteredOutPortfolioNetworkKeys(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.filteredOutPortfolioNetworkKeys = payload
      },

      setFilteredOutPortfolioAccountIds(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.filteredOutPortfolioAccountIds = payload
      },

      setHidePortfolioSmallBalances(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.hidePortfolioSmallBalances = payload
      },

      setShowNetworkLogoOnNfts(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.showNetworkLogoOnNfts = payload
      },

      setHidePortfolioBalances(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.hidePortfolioBalances = payload
      },

      setHidePortfolioNFTsTab(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.hidePortfolioNFTsTab = payload
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
