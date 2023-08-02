/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BraveWallet,
  DefaultCurrencies,
  NetworkFilterType,
  OriginInfo,
  RefreshOpts,
  SolFeeEstimates,
  UpdateAccountNamePayloadType,
  WalletState,
  WalletInitializedPayload
} from '../../constants/types'
import {
  AddSitePermissionPayloadType,
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged,
  DefaultEthereumWalletChanged,
  DefaultSolanaWalletChanged,
  GetCoinMarketPayload,
  GetCoinMarketsResponse,
  RemoveSitePermissionPayloadType,
  SetUserAssetVisiblePayloadType,
  SitePermissionsPayloadType,
  UnlockWalletPayloadType,
  UpdateUsetAssetType
} from '../constants/action_types'
import {
  AddAccountPayloadType,
  RemoveAccountPayloadType
} from '../../page/constants/action_types'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

// Utils
import {
  parseJSONFromLocalStorage,
  makeInitialFilteredOutNetworkKeys
} from '../../utils/local-storage-utils'
import { findAccountByUniqueKey } from '../../utils/account-utils'

// Options
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import {
  NoneGroupByOption
} from '../../options/group-assets-by-options'
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { AllAccountsOptionUniqueKey } from '../../options/account-filter-options'
import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

const defaultState: WalletState = {
  hasInitialized: false,
  isFilecoinEnabled: false,
  isSolanaEnabled: false,
  isBitcoinEnabled: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  accounts: [],
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
    originSpec: '',
  },
  gasEstimates: undefined,
  connectedAccounts: [],
  isMetaMaskInstalled: false,
  defaultCurrencies: {
    fiat: '',
    crypto: ''
  },
  isLoadingCoinMarketData: true,
  coinMarketData: [],
  selectedNetworkFilter: parseJSONFromLocalStorage(
    'PORTFOLIO_NETWORK_FILTER_OPTION',
    AllNetworksOptionDefault
  ),
  selectedAssetFilter:
    window.localStorage.getItem(
      LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION
    ) || HighToLowAssetsFilterOption.id,
  selectedGroupAssetsByItem:
    window
      .localStorage
      .getItem(
        LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY
      ) ||
    NoneGroupByOption.id,
  selectedAccountFilter: AllAccountsOptionUniqueKey,
  solFeeEstimates: undefined,
  onRampCurrencies: [] as BraveWallet.OnRampCurrency[],
  selectedCurrency: undefined,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  isPanelV2FeatureEnabled: false,
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
  hidePortfolioNFTsTab:
    window.localStorage.getItem(LOCAL_STORAGE_KEYS.HIDE_PORTFOLIO_NFTS_TAB) ===
    'true',
  removedNonFungibleTokens: [] as BraveWallet.BlockchainToken[],
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
  isRefreshingNetworksAndTokens: false
}

// async actions
export const WalletAsyncActions = {
  initialize: createAction<RefreshOpts>('initialize'),
  refreshAll: createAction<RefreshOpts>('refreshAll'),
  lockWallet: createAction('lockWallet'), // keyringService.lock()
  unlockWallet: createAction<UnlockWalletPayloadType>('unlockWallet'),
  addFavoriteApp: createAction<BraveWallet.AppItem>('addFavoriteApp'), // should use ApiProxy.walletHandler + refreshWalletInfo
  removeFavoriteApp: createAction<BraveWallet.AppItem>('removeFavoriteApp'), // should use ApiProxy.walletHandler + refreshWalletInfo
  addUserAsset: createAction<BraveWallet.BlockchainToken>('addUserAsset'),
  updateUserAsset: createAction<UpdateUsetAssetType>('updateUserAsset'),
  removeUserAsset: createAction<BraveWallet.BlockchainToken>('removeUserAsset'),
  setUserAssetVisible: createAction<SetUserAssetVisiblePayloadType>(
    'setUserAssetVisible'
  ), // alias for ApiProxy.braveWalletService.setUserAssetVisible
  selectAccount: createAction<BraveWallet.AccountId>('selectAccount'), // should use apiProxy - keyringService
  getAllNetworks: createAction('getAllNetworks'), // alias to refreshFullNetworkList
  keyringCreated: createAction('keyringCreated'),
  keyringRestored: createAction('keyringRestored'),
  keyringReset: createAction('keyringReset'),
  locked: createAction('locked'),
  unlocked: createAction('unlocked'),
  backedUp: createAction('backedUp'),
  accountsChanged: createAction('accountsChanged'),
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
  addSitePermission:
    createAction<AddSitePermissionPayloadType>('addSitePermission'),
  refreshNetworksAndTokens:
    createAction<RefreshOpts>('refreshNetworksAndTokens'),
  expandWalletNetworks: createAction('expandWalletNetworks'), // replace with chrome.tabs.create helper
  refreshBalancesAndPriceHistory: createAction(
    'refreshBalancesAndPriceHistory'
  ),
  getCoinMarkets: createAction<GetCoinMarketPayload>('getCoinMarkets'),
  setSelectedNetworkFilter: createAction<NetworkFilterType>(
    'setSelectedNetworkFilter'
  ),
  setSelectedAccountFilterItem: createAction<string>(
    'setSelectedAccountFilterItem'
  ),
  addAccount: createAction<AddAccountPayloadType>('addAccount'), // alias for keyringService.addAccount
  getOnRampCurrencies: createAction('getOnRampCurrencies'),
  autoLockMinutesChanged: createAction('autoLockMinutesChanged'), // No reducer or API logic for this (UNUSED)
  updateTokenPinStatus: createAction<BraveWallet.BlockchainToken>(
    'updateTokenPinStatus'
  ),
  updateAccountName: createAction<UpdateAccountNamePayloadType>(
    'updateAccountName'
  ),
  removeAccount: createAction<RemoveAccountPayloadType>(
    'removeAccount'
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
        { payload }: PayloadAction<OriginInfo>
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
        state.accounts = payload.allAccounts.accounts
        state.isWalletCreated = payload.walletInfo.isWalletCreated
        state.isFilecoinEnabled = payload.walletInfo.isFilecoinEnabled
        state.isSolanaEnabled = payload.walletInfo.isSolanaEnabled
        state.isBitcoinEnabled = payload.walletInfo.isBitcoinEnabled
        state.isWalletLocked = payload.walletInfo.isWalletLocked
        state.isWalletBackedUp = payload.walletInfo.isWalletBackedUp
        state.isNftPinningFeatureEnabled =
          payload.walletInfo.isNftPinningFeatureEnabled
        state.isPanelV2FeatureEnabled =
          payload.walletInfo.isPanelV2FeatureEnabled
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
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) {
        state.assetAutoDiscoveryCompleted = true
      },

      setCoinMarkets: (
        state: WalletState,
        { payload }: PayloadAction<GetCoinMarketsResponse>
      ) => {
        state.coinMarketData = payload.success
          ? payload.values.map((coin) => {
              coin.image = coin.image.replace(
                'https://assets.coingecko.com',
                ' https://assets.cgproxy.brave.com'
              )
              return coin
            })
          : []
        state.isLoadingCoinMarketData = false
      },

      selectCurrency(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.OnRampCurrency>
      ) {
        state.selectedCurrency = payload
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

      setOnRampCurrencies(
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.OnRampCurrency[]>
      ) {
        state.onRampCurrencies = payload
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

      refreshAccountInfo: (
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.AllAccountsInfo>
      ) => {
        state.accounts.forEach((account) => {
          const info = findAccountByUniqueKey(payload.accounts, account.accountId.uniqueKey)
          if (info) {
            account.name = info.name
          }
        })
      },
      setIsRefreshingNetworksAndTokens: (
        state: WalletState,
        { payload }: PayloadAction<boolean>
      ) => {
        state.isRefreshingNetworksAndTokens = payload
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
