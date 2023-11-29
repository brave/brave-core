/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BraveWallet,
  WalletState,
  WalletInitializedPayload,
  DefaultCurrencies,
  SolFeeEstimates,
  NetworkFilterType,
  RefreshOpts,
  UpdateAccountNamePayloadType,
  ImportAccountErrorType
} from '../../constants/types'
import {
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged,
  DefaultEthereumWalletChanged,
  DefaultSolanaWalletChanged,
  RemoveSitePermissionPayloadType,
  SetUserAssetVisiblePayloadType,
  SitePermissionsPayloadType,
  UnlockWalletPayloadType,
  UpdateUsetAssetType
} from '../constants/action_types'
import {
  ImportAccountFromJsonPayloadType,
  ImportAccountPayloadType,
  RemoveAccountPayloadType
} from '../../page/constants/action_types'
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
import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

const defaultState: WalletState = {
  hasInitialized: false,
  allowNewWalletFilecoinAccount: true,
  isBitcoinEnabled: false,
  isZCashEnabled: false,
  isWalletCreated: false,
  isWalletLocked: true,
  hasIncorrectPassword: false,
  userVisibleTokensInfo: [],
  fullTokenList: [],
  selectedPortfolioTimeline:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION
    ) !== undefined
      ? Number(
          window.localStorage.getItem(
            LOCAL_STORAGE_KEYS.PORTFOLIO_TIME_LINE_OPTION
          )
        )
      : BraveWallet.AssetPriceTimeframe.OneDay,
  addUserAssetError: false,
  defaultEthereumWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  defaultSolanaWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  activeOrigin: {
    eTldPlusOne: '',
    originSpec: ''
  },
  gasEstimates: undefined,
  connectedAccounts: [],
  isMetaMaskInstalled: false,
  defaultCurrencies: {
    fiat: '',
    crypto: ''
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
  solFeeEstimates: undefined,
  selectedDepositAssetId: undefined,
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
  removedFungibleTokenIds: JSON.parse(
    localStorage.getItem(LOCAL_STORAGE_KEYS.USER_REMOVED_FUNGIBLE_TOKEN_IDS) ||
      '[]'
  ),
  removedNonFungibleTokenIds: JSON.parse(
    localStorage.getItem(
      LOCAL_STORAGE_KEYS.USER_REMOVED_NON_FUNGIBLE_TOKEN_IDS
    ) || '[]'
  ),
  deletedNonFungibleTokenIds: JSON.parse(
    localStorage.getItem(
      LOCAL_STORAGE_KEYS.USER_DELETED_NON_FUNGIBLE_TOKEN_IDS
    ) || '[]'
  ),
  hidePortfolioNFTsTab:
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB) ===
    'true',
  removedNonFungibleTokens: [] as BraveWallet.BlockchainToken[],
  deletedNonFungibleTokens: [] as BraveWallet.BlockchainToken[],
  filteredOutPortfolioNetworkKeys: parseJSONFromLocalStorage(
    'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS',
    makeInitialFilteredOutNetworkKeys()
  ),
  filteredOutPortfolioAccountAddresses: parseJSONFromLocalStorage(
    'FILTERED_OUT_PORTFOLIO_ACCOUNT_ADDRESSES',
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
  isRefreshingNetworksAndTokens: false,
  importAccountError: undefined
}

// async actions
export const WalletAsyncActions = {
  initialize: createAction<RefreshOpts>('initialize'),
  refreshAll: createAction<RefreshOpts>('refreshAll'),
  lockWallet: createAction('lockWallet'), // keyringService.lock()
  unlockWallet: createAction<UnlockWalletPayloadType>('unlockWallet'),
  addUserAsset: createAction<BraveWallet.BlockchainToken>('addUserAsset'),
  updateUserAsset: createAction<UpdateUsetAssetType>('updateUserAsset'),
  removeUserAsset: createAction<BraveWallet.BlockchainToken>('removeUserAsset'),
  setUserAssetVisible: createAction<SetUserAssetVisiblePayloadType>(
    'setUserAssetVisible'
  ), // alias for ApiProxy.braveWalletService.setUserAssetVisible
  selectAccount: createAction<BraveWallet.AccountId>('selectAccount'), // should use apiProxy - keyringService
  getAllNetworks: createAction('getAllNetworks'), // alias to refreshFullNetworkList
  walletCreated: createAction('walletCreated'),
  walletRestored: createAction('walletRestored'),
  walletReset: createAction('walletReset'),
  locked: createAction('locked'),
  unlocked: createAction('unlocked'),
  backedUp: createAction('backedUp'),
  getAllTokensList: createAction('getAllTokensList'),
  selectPortfolioTimeline: createAction<BraveWallet.AssetPriceTimeframe>(
    'selectPortfolioTimeline'
  ),
  defaultEthereumWalletChanged: createAction<DefaultEthereumWalletChanged>(
    'defaultEthereumWalletChanged'
  ), // refreshWalletInfo
  defaultSolanaWalletChanged: createAction<DefaultSolanaWalletChanged>(
    'defaultSolanaWalletChanged'
  ), // refreshWalletInfo
  defaultBaseCurrencyChanged: createAction<DefaultBaseCurrencyChanged>(
    'defaultBaseCurrencyChanged'
  ), // refreshWalletInfo
  defaultBaseCryptocurrencyChanged:
    createAction<DefaultBaseCryptocurrencyChanged>(
      'defaultBaseCryptocurrencyChanged'
    ),
  removeSitePermission: createAction<RemoveSitePermissionPayloadType>(
    'removeSitePermission'
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
  autoLockMinutesChanged: createAction('autoLockMinutesChanged'), // No reducer or API logic for this (UNUSED)
  updateAccountName:
    createAction<UpdateAccountNamePayloadType>('updateAccountName'),
  removeAccount: createAction<RemoveAccountPayloadType>('removeAccount'),
  importAccount: createAction<ImportAccountPayloadType>('importAccount'),
  importAccountFromJson: createAction<ImportAccountFromJsonPayloadType>(
    'importAccountFromJson'
  )
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

      addUserAssetError(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.addUserAssetError = payload
      },

      defaultCurrenciesUpdated(
        state: WalletState,
        { payload }: PayloadAction<DefaultCurrencies>
      ) {
        state.defaultCurrencies = payload
      },

      defaultEthereumWalletUpdated(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.DefaultWallet>
      ) {
        state.defaultEthereumWallet = payload
      },

      defaultSolanaWalletUpdated(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.DefaultWallet>
      ) {
        state.defaultSolanaWallet = payload
      },

      hasIncorrectPassword(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.hasIncorrectPassword = payload
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

      portfolioTimelineUpdated(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.AssetPriceTimeframe>
      ) {
        state.selectedPortfolioTimeline = payload
      },

      setAllTokensList: (
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) => {
        state.fullTokenList = payload
      },

      setAssetAutoDiscoveryCompleted(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.assetAutoDiscoveryCompleted = payload
      },

      selectOnRampAssetId(
        state: WalletState,
        { payload }: PayloadAction<string | undefined>
      ) {
        state.selectedDepositAssetId = payload
      },

      setGasEstimates(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.GasEstimation1559>
      ) {
        state.hasFeeEstimatesError = false
        state.gasEstimates = payload
      },

      setMetaMaskInstalled(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.isMetaMaskInstalled = payload
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

      setRemovedFungibleTokenIds(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.removedFungibleTokenIds = payload
      },

      setRemovedNonFungibleTokenIds(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.removedNonFungibleTokenIds = payload
      },

      setRemovedNonFungibleTokens(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) {
        state.removedNonFungibleTokens = payload
      },

      setDeletedNonFungibleTokenIds(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.deletedNonFungibleTokenIds = payload
      },

      setDeletedNonFungibleTokens(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) {
        state.deletedNonFungibleTokens = payload
      },

      setFilteredOutPortfolioNetworkKeys(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.filteredOutPortfolioNetworkKeys = payload
      },

      setFilteredOutPortfolioAccountAddresses(
        state: WalletState,
        { payload }: PayloadAction<string[]>
      ) {
        state.filteredOutPortfolioAccountAddresses = payload
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

      setSitePermissions(
        state: WalletState,
        { payload }: PayloadAction<SitePermissionsPayloadType>
      ) {
        state.connectedAccounts = payload.accounts
      },

      setSolFeeEstimates(
        state: WalletState,
        { payload }: PayloadAction<SolFeeEstimates>
      ) {
        state.hasFeeEstimatesError = false
        state.solFeeEstimates = payload
      },

      setHasFeeEstimatesError: (
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) => {
        state.hasFeeEstimatesError = payload
      },

      setVisibleTokensInfo: (
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) => {
        state.userVisibleTokensInfo = payload
      },

      setIsRefreshingNetworksAndTokens: (
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) => {
        state.isRefreshingNetworksAndTokens = payload
      },
      setImportAccountError(
        state: WalletState,
        { payload }: PayloadAction<ImportAccountErrorType>
      ) {
        state.importAccountError = payload
      },
      setAllowNewWalletFilecoinAccount(
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) {
        state.allowNewWalletFilecoinAccount = payload
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
