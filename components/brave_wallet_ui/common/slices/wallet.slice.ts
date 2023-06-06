/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  BraveWallet,
  GetBlockchainTokenBalanceReturnInfo,
  GetPriceHistoryReturnInfo,
  PortfolioTokenHistoryAndInfo,
  WalletAccountType,
  WalletState,
  WalletInfo,
  DefaultCurrencies,
  GetPriceReturnInfo,
  GetNativeAssetBalancesPayload,
  SolFeeEstimates,
  SerializableOriginInfo,
  NetworkFilterType,
  RefreshOpts
} from '../../constants/types'
import {
  AddSitePermissionPayloadType,
  ChainChangedEventPayloadType,
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged,
  DefaultEthereumWalletChanged,
  DefaultSolanaWalletChanged,
  GetCoinMarketPayload,
  GetCoinMarketsResponse,
  RemoveSitePermissionPayloadType,
  SelectedAccountChangedPayloadType,
  SetUserAssetVisiblePayloadType,
  SitePermissionsPayloadType,
  UnlockWalletPayloadType,
  UpdateUsetAssetType
} from '../constants/action_types'
import {
  AddAccountPayloadType,
  AddBitcoinAccountPayloadType
} from '../../page/constants/action_types'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

// Utils
import { mojoTimeDeltaToJSDate } from '../../../common/mojomUtils'
import Amount from '../../utils/amount'
import {
  createTokenBalanceRegistryKey,
  findAccountInList
} from '../../utils/account-utils'
import {
  parseJSONFromLocalStorage,
  makeInitialFilteredOutNetworkKeys
} from '../../utils/local-storage-utils'

// Options
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import {
  AccountsGroupByOption
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
  selectedAccount: {} as WalletAccountType,
  accounts: [],
  userVisibleTokensInfo: [],
  fullTokenList: [],
  portfolioPriceHistory: [],
  isFetchingPortfolioPriceHistory: true,
  selectedPortfolioTimeline: window
    .localStorage
    .getItem(
      LOCAL_STORAGE_KEYS
        .PORTFOLIO_TIME_LINE_OPTION
    ) !== undefined
    ? Number(
      window
        .localStorage
        .getItem(
          LOCAL_STORAGE_KEYS
            .PORTFOLIO_TIME_LINE_OPTION
        )
    )
    : BraveWallet.AssetPriceTimeframe.OneDay,
  transactionSpotPrices: [],
  addUserAssetError: false,
  defaultEthereumWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  defaultSolanaWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
  activeOrigin: {
    eTldPlusOne: '',
    originSpec: '',
    origin: {
      scheme: '',
      host: '',
      port: 0,
      nonceIfOpaque: undefined
    }
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
  selectedNetworkFilter: parseJSONFromLocalStorage('PORTFOLIO_NETWORK_FILTER_OPTION', AllNetworksOptionDefault),
  selectedAssetFilter: window.localStorage.getItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION) || HighToLowAssetsFilterOption.id,
  selectedGroupAssetsByItem:
    window
      .localStorage
      .getItem(
        LOCAL_STORAGE_KEYS.GROUP_PORTFOLIO_ASSETS_BY
      ) ||
      AccountsGroupByOption.id,
  selectedAccountFilter: window.localStorage.getItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ACCOUNT_FILTER_OPTION) || AllAccountsOptionUniqueKey,
  solFeeEstimates: undefined,
  onRampCurrencies: [] as BraveWallet.OnRampCurrency[],
  selectedCurrency: undefined,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  isPanelV2FeatureEnabled: false,
  hidePortfolioGraph: window
    .localStorage
    .getItem(
      LOCAL_STORAGE_KEYS
        .IS_PORTFOLIO_OVERVIEW_GRAPH_HIDDEN
    ) === 'true',
  hidePortfolioBalances: window
    .localStorage
    .getItem(
      LOCAL_STORAGE_KEYS
        .HIDE_PORTFOLIO_BALANCES
    ) === 'true',
  removedFungibleTokenIds:
    JSON.parse(
      localStorage
        .getItem(
          LOCAL_STORAGE_KEYS
            .USER_REMOVED_FUNGIBLE_TOKEN_IDS
        ) || '[]'),
  removedNonFungibleTokenIds:
    JSON.parse(
      localStorage
        .getItem(
          LOCAL_STORAGE_KEYS
            .USER_REMOVED_NON_FUNGIBLE_TOKEN_IDS
        ) || '[]'),
  hidePortfolioNFTsTab: window
  .localStorage
  .getItem(
    LOCAL_STORAGE_KEYS
      .HIDE_PORTFOLIO_NFTS_TAB
  ) === 'true',
  removedNonFungibleTokens: [] as BraveWallet.BlockchainToken[],
  filteredOutPortfolioNetworkKeys:
    parseJSONFromLocalStorage(
      'FILTERED_OUT_PORTFOLIO_NETWORK_KEYS',
      makeInitialFilteredOutNetworkKeys()
    ),
  filteredOutPortfolioAccountAddresses:
    parseJSONFromLocalStorage(
      'FILTERED_OUT_PORTFOLIO_ACCOUNT_ADDRESSES',
      []
    ),
  hidePortfolioSmallBalances: window
    .localStorage
    .getItem(
      LOCAL_STORAGE_KEYS
        .HIDE_PORTFOLIO_SMALL_BALANCES
    ) === 'true',
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
  chainChangedEvent:
    createAction<ChainChangedEventPayloadType>('chainChangedEvent'),
  keyringCreated: createAction('keyringCreated'),
  keyringRestored: createAction('keyringRestored'),
  keyringReset: createAction('keyringReset'),
  locked: createAction('locked'),
  unlocked: createAction('unlocked'),
  backedUp: createAction('backedUp'),
  accountsChanged: createAction('accountsChanged'),
  selectedAccountChanged: createAction<SelectedAccountChangedPayloadType>(
    'selectedAccountChanged'
  ),
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
  addBitcoinAccount:
    createAction<AddBitcoinAccountPayloadType>('addBitcoinAccount'),
  getOnRampCurrencies: createAction('getOnRampCurrencies'),
  autoLockMinutesChanged: createAction('autoLockMinutesChanged'), // No reducer or API logic for this (UNUSED)
  updateTokenPinStatus: createAction<BraveWallet.BlockchainToken>('updateTokenPinStatus')
}

// slice
export const createWalletSlice = (initialState: WalletState = defaultState) => {
  return createSlice({
    name: 'wallet',
    initialState,
    reducers: {
      activeOriginChanged (state: WalletState, { payload }: PayloadAction<SerializableOriginInfo>) {
        state.activeOrigin = payload
      },

      addUserAssetError (state: WalletState, { payload }: PayloadAction<boolean>) {
        state.addUserAssetError = payload
      },

      defaultCurrenciesUpdated (state: WalletState, { payload }: PayloadAction<DefaultCurrencies>) {
        state.defaultCurrencies = payload
      },

      defaultEthereumWalletUpdated (state: WalletState, { payload }: PayloadAction<BraveWallet.DefaultWallet>) {
        state.defaultEthereumWallet = payload
      },

      defaultSolanaWalletUpdated (state: WalletState, { payload }: PayloadAction<BraveWallet.DefaultWallet>) {
        state.defaultSolanaWallet = payload
      },

      hasIncorrectPassword (state: WalletState, { payload }: PayloadAction<boolean>) {
        state.hasIncorrectPassword = payload
      },

      initialized (state: WalletState, { payload }: PayloadAction<WalletInfo>) {
        const accounts = payload.accountInfos.map(
          (info: BraveWallet.AccountInfo, idx: number): WalletAccountType => {
            return {
              ...info,
              tokenBalanceRegistry: {},
              nativeBalanceRegistry: {},
            }
          }
        )

        const selectedAccount = payload.selectedAccount
          ? accounts.find(
              (account) =>
                account.address.toLowerCase() ===
                payload.selectedAccount.toLowerCase()
            ) ?? accounts[0]
          : accounts[0]
        state.hasInitialized = true
        state.isWalletCreated = payload.isWalletCreated
        state.isFilecoinEnabled = payload.isFilecoinEnabled
        state.isSolanaEnabled = payload.isSolanaEnabled
        state.isBitcoinEnabled = payload.isBitcoinEnabled
        state.isWalletLocked = payload.isWalletLocked
        state.accounts = accounts
        state.isWalletBackedUp = payload.isWalletBackedUp
        state.selectedAccount = selectedAccount
        state.isNftPinningFeatureEnabled = payload.isNftPinningFeatureEnabled
        state.isPanelV2FeatureEnabled = payload.isPanelV2FeatureEnabled
      },

      nativeAssetBalancesUpdated: (
        state: WalletState,
        { payload }: PayloadAction<GetNativeAssetBalancesPayload>
      ) => {
        state.accounts.forEach((account, accountIndex) => {
          payload.balances[accountIndex].forEach((info, tokenIndex) => {
            if (info.error === BraveWallet.ProviderError.kSuccess) {
              state.accounts[accountIndex].nativeBalanceRegistry[info.chainId] =
                Amount.normalize(info.balance)
            }
          })
        })

        state.selectedAccount = findAccountInList(state.selectedAccount, state.accounts)
      },

      portfolioPriceHistoryUpdated: (
        state: WalletState,
        { payload }: PayloadAction<PortfolioTokenHistoryAndInfo[][]>
      ) => {
        const history = payload.map((infoArray) => {
          return infoArray.map((info) => {
            if (new Amount(info.balance).isPositive() && info.token.visible) {
              return info.history.values.map((value) => {
                return {
                  date: value.date,
                  price: new Amount(info.balance)
                    .divideByDecimals(info.token.decimals)
                    .times(value.price)
                    .toNumber()
                }
              })
            } else {
              return []
            }
          })
        })
        const jointHistory = [].concat
          .apply([], [...history])
          .filter((h: []) => h.length > 1) as GetPriceHistoryReturnInfo[][]

        // Since the Price History API sometimes will return a shorter
        // array of history, this checks for the shortest array first to
        // then map and reduce to it length
        const shortestHistory =
          jointHistory.length > 0
            ? jointHistory.reduce((a, b) => (a.length <= b.length ? a : b))
            : []
        const sumOfHistory =
          jointHistory.length > 0
            ? shortestHistory.map((token, tokenIndex) => {
                return {
                  date: mojoTimeDeltaToJSDate(token.date),
                  close: jointHistory
                    .map((price) => Number(price[tokenIndex].price) || 0)
                    .reduce((sum, x) => sum + x, 0)
                }
              })
            : []

        state.portfolioPriceHistory = sumOfHistory
        state.isFetchingPortfolioPriceHistory = sumOfHistory.length === 0
      },

      portfolioTimelineUpdated (state: WalletState, { payload }: PayloadAction<BraveWallet.AssetPriceTimeframe>) {
        state.isFetchingPortfolioPriceHistory = true
        state.selectedPortfolioTimeline = payload
      },

      pricesUpdated (state: WalletState, { payload }: PayloadAction<GetPriceReturnInfo>) {
        if (payload.success) {
          state.transactionSpotPrices = payload.values
        }
      },

      setAllTokensList: (
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.BlockchainToken[]>
      ) => {
        state.fullTokenList = payload
      },

      setAssetAutoDiscoveryCompleted (state: WalletState, { payload }: PayloadAction<BraveWallet.BlockchainToken[]>) {
        state.assetAutoDiscoveryCompleted = true
      },

      setCoinMarkets: (
        state: WalletState,
        { payload }: PayloadAction<GetCoinMarketsResponse>
      ) => {
        state.coinMarketData = payload.success
          ? payload.values.map(coin => {
            coin.image = coin.image.replace('https://assets.coingecko.com', ' https://assets.cgproxy.brave.com')
            return coin
          })
          : []
        state.isLoadingCoinMarketData = false
      },

      selectCurrency (state: WalletState, { payload }: PayloadAction<BraveWallet.OnRampCurrency>) {
        state.selectedCurrency = payload
      },

      setGasEstimates (state: WalletState, { payload }: PayloadAction<BraveWallet.GasEstimation1559>) {
        state.hasFeeEstimatesError = false
        state.gasEstimates = payload
      },

      setMetaMaskInstalled (state: WalletState, { payload }: PayloadAction<boolean>) {
        state.isMetaMaskInstalled = payload
      },


      setOnRampCurrencies (state: WalletState, { payload }: PayloadAction<BraveWallet.OnRampCurrency[]>) {
        state.onRampCurrencies = payload
      },

      setPasswordAttempts (state: WalletState, { payload }: PayloadAction<number>) {
        state.passwordAttempts = payload
      },

      setSelectedAccount (state: WalletState, { payload }: PayloadAction<WalletAccountType>) {
        state.selectedAccount = payload
      },

      setSelectedAssetFilterItem (state: WalletState, { payload }: PayloadAction<string>) {
        state.selectedAssetFilter = payload
      },

      setSelectedGroupAssetsByItem (
        state: WalletState,
        { payload }: PayloadAction<string>
      ) {
        state.selectedGroupAssetsByItem = payload
      },

      setHidePortfolioGraph
        (
          state: WalletState,
          { payload }: PayloadAction<boolean>
        ) {
        state.hidePortfolioGraph = payload
      },

      setRemovedFungibleTokenIds
        (
          state: WalletState,
          { payload }: PayloadAction<string[]>
        ) {
        state.removedFungibleTokenIds = payload
      },

      setRemovedNonFungibleTokenIds
        (
          state: WalletState,
          { payload }: PayloadAction<string[]>
        ) {
        state.removedNonFungibleTokenIds = payload
      },

      setRemovedNonFungibleTokens (state: WalletState, { payload }: PayloadAction<BraveWallet.BlockchainToken[]>) {
        state.removedNonFungibleTokens = payload
      },

      setFilteredOutPortfolioNetworkKeys
        (
          state: WalletState,
          { payload }: PayloadAction<string[]>
        ) {
        state.filteredOutPortfolioNetworkKeys = payload
      },

      setFilteredOutPortfolioAccountAddresses
        (
          state: WalletState,
          { payload }: PayloadAction<string[]>
        ) {
        state.filteredOutPortfolioAccountAddresses = payload
      },

      setHidePortfolioSmallBalances
        (
          state: WalletState,
          { payload }: PayloadAction<boolean>
        ) {
        state.hidePortfolioSmallBalances = payload
      },

      setIsFetchingPortfolioPriceHistory
        (
          state: WalletState,
          { payload }: PayloadAction<boolean>
        ) {
        state.isFetchingPortfolioPriceHistory = payload
      },

      setHidePortfolioBalances
        (
          state: WalletState,
          { payload }: PayloadAction<boolean>
        ) {
        state.hidePortfolioBalances = payload
      },

      setHidePortfolioNFTsTab
        (
          state: WalletState,
          { payload }: PayloadAction<boolean>
        ) {
        state.hidePortfolioNFTsTab = payload
      },

      setSitePermissions (state: WalletState, { payload }: PayloadAction<SitePermissionsPayloadType>) {
        state.connectedAccounts = payload.accounts
      },

      setSolFeeEstimates (state: WalletState, { payload }: PayloadAction<SolFeeEstimates>) {
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

      tokenBalancesUpdated: (
        state: WalletState,
        { payload }: PayloadAction<GetBlockchainTokenBalanceReturnInfo>
      ) => {
        const visibleTokens = state.userVisibleTokensInfo.filter(
          (asset) => asset.contractAddress !== ''
        )

        state.accounts.forEach((account, accountIndex) => {
          payload.balances[accountIndex]?.forEach((info, tokenIndex) => {
            if (info.error === BraveWallet.ProviderError.kSuccess) {
              const token = visibleTokens[tokenIndex]
              const registryKey = createTokenBalanceRegistryKey(token)
              state.accounts[accountIndex].tokenBalanceRegistry[registryKey] =
                Amount.normalize(info.balance)
            }
          })
        })

        state.selectedAccount = findAccountInList(state.selectedAccount, state.accounts)
      },

      refreshAccountInfo: (
        state: WalletState,
        { payload }: PayloadAction<BraveWallet.AllAccountsInfo>
      ) => {
        state.accounts.forEach((account) => {
          const info = payload.accounts.find((info) => {
            return account.address === info.address
          })
          if (info) {
            account.name = info.name
          }
        })

        state.selectedAccount = findAccountInList(
          state.selectedAccount,
          state.accounts
        )
      },
    },
    extraReducers: (builder) => {
      builder.addCase(WalletAsyncActions.locked.type, (state) => {
        state.isWalletLocked = true
      })

      builder.addCase(WalletAsyncActions.setSelectedAccountFilterItem, (state, { payload }) => {
        state.isFetchingPortfolioPriceHistory = true
        state.selectedAccountFilter = payload
      })

      builder.addCase(
        WalletAsyncActions.setSelectedNetworkFilter,
        (state, { payload }) => {
          state.isFetchingPortfolioPriceHistory = true
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
