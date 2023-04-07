/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AccountTransactions,
  BraveWallet,
  GetBlockchainTokenBalanceReturnInfo,
  GetPriceHistoryReturnInfo,
  PortfolioTokenHistoryAndInfo,
  WalletAccountType,
  WalletState,
  WalletInfoBase,
  WalletInfo,
  DefaultCurrencies,
  GetPriceReturnInfo,
  GetNativeAssetBalancesPayload,
  SolFeeEstimates,
  ApproveERC20Params,
  ER20TransferParams,
  ERC721TransferFromParams,
  SendTransactionParams,
  SPLTransferFromParams,
  SerializableTransactionInfo,
  SerializableOriginInfo,
  NetworkFilterType
} from '../../constants/types'
import {
  AddSitePermissionPayloadType,
  CancelTransactionPayload,
  ChainChangedEventPayloadType,
  DefaultBaseCryptocurrencyChanged,
  DefaultBaseCurrencyChanged,
  DefaultEthereumWalletChanged,
  DefaultSolanaWalletChanged,
  GetCoinMarketPayload,
  GetCoinMarketsResponse,
  NewUnapprovedTxAdded,
  RemoveSitePermissionPayloadType,
  RetryTransactionPayload,
  SelectedAccountChangedPayloadType,
  SetTransactionProviderErrorType,
  SetUserAssetVisiblePayloadType,
  SitePermissionsPayloadType,
  SpeedupTransactionPayload,
  TransactionStatusChanged,
  UnapprovedTxUpdated,
  UnlockWalletPayloadType,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionNonceType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  UpdateUsetAssetType
} from '../constants/action_types'
import {
  AddAccountPayloadType,
  AddFilecoinAccountPayloadType
} from '../../page/constants/action_types'
import { LOCAL_STORAGE_KEYS } from '../../common/constants/local-storage-keys'

// Utils
import { mojoTimeDeltaToJSDate } from '../../../common/mojomUtils'
import { sortTransactionByDate } from '../../utils/tx-utils'
import Amount from '../../utils/amount'
import {
  createTokenBalanceRegistryKey,
  getAccountType
} from '../../utils/account-utils'
import { parseJSONFromLocalStorage } from '../../utils/local-storage-utils'

// Options
import { HighToLowAssetsFilterOption } from '../../options/asset-filter-options'
import { AllNetworksOptionDefault } from '../../options/network-filter-options'
import { AllAccountsOption } from '../../options/account-filter-options'
import { createAction, createSlice, PayloadAction } from '@reduxjs/toolkit'

const defaultState: WalletState = {
  hasInitialized: false,
  isFilecoinEnabled: false,
  isSolanaEnabled: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  accounts: [],
  userVisibleTokensInfo: [],
  transactions: {},
  pendingTransactions: [],
  knownTransactions: [],
  fullTokenList: [],
  portfolioPriceHistory: [],
  selectedPendingTransaction: undefined,
  isFetchingPortfolioPriceHistory: true,
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe.OneDay,
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
  transactionProviderErrorRegistry: {},
  isLoadingCoinMarketData: true,
  coinMarketData: [],
  defaultAccounts: [] as BraveWallet.AccountInfo[],
  selectedNetworkFilter: parseJSONFromLocalStorage('PORTFOLIO_NETWORK_FILTER_OPTION', AllNetworksOptionDefault),
  selectedAssetFilter: window.localStorage.getItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ASSET_FILTER_OPTION) || HighToLowAssetsFilterOption.id,
  selectedAccountFilter: window.localStorage.getItem(LOCAL_STORAGE_KEYS.PORTFOLIO_ACCOUNT_FILTER_OPTION) || AllAccountsOption.id,
  solFeeEstimates: undefined,
  onRampCurrencies: [] as BraveWallet.OnRampCurrency[],
  selectedCurrency: undefined,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isNftPinningFeatureEnabled: false,
  isPanelV2FeatureEnabled: false
}

// async actions
export const WalletAsyncActions = {
  initialize: createAction('initialize'),
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
  selectAccount: createAction<WalletAccountType>('selectAccount'), // should use apiProxy - keyringService
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
  sendTransaction: createAction<SendTransactionParams>('sendTransaction'),
  sendERC20Transfer: createAction<ER20TransferParams>('sendERC20Transfer'),
  sendSPLTransfer: createAction<SPLTransferFromParams>('sendSPLTransfer'),
  sendERC721TransferFrom: createAction<ERC721TransferFromParams>(
    'sendERC721TransferFrom'
  ),
  approveERC20Allowance: createAction<ApproveERC20Params>(
    'approveERC20Allowance'
  ),
  transactionStatusChanged: createAction<TransactionStatusChanged>(
    'transactionStatusChanged'
  ),
  approveTransaction:
    createAction<SerializableTransactionInfo>('approveTransaction'),
  rejectTransaction:
    createAction<SerializableTransactionInfo>('rejectTransaction'),
  rejectAllTransactions: createAction('rejectAllTransactions'),
  refreshGasEstimates: createAction<SerializableTransactionInfo>(
    'refreshGasEstimates'
  ),
  updateUnapprovedTransactionGasFields:
    createAction<UpdateUnapprovedTransactionGasFieldsType>(
      'updateUnapprovedTransactionGasFields'
    ),
  updateUnapprovedTransactionSpendAllowance:
    createAction<UpdateUnapprovedTransactionSpendAllowanceType>(
      'updateUnapprovedTransactionSpendAllowance'
    ),
  updateUnapprovedTransactionNonce:
    createAction<UpdateUnapprovedTransactionNonceType>(
      'updateUnapprovedTransactionNonce'
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
  refreshBalancesAndPrices: createAction('refreshBalancesAndPrices'),
  refreshNetworksAndTokens: createAction('refreshNetworksAndTokens'),
  retryTransaction: createAction<RetryTransactionPayload>('retryTransaction'),
  cancelTransaction:
    createAction<CancelTransactionPayload>('cancelTransaction'),
  speedupTransaction:
    createAction<SpeedupTransactionPayload>('speedupTransaction'),
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
  // alias for keyringService.addFilecoinAccount
  addFilecoinAccount:
    createAction<AddFilecoinAccountPayloadType>('addFilecoinAccount'),
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
              id: `${idx + 1}`,
              name: info.name,
              address: info.address,
              accountType: getAccountType(info),
              deviceId: info.hardware ? info.hardware.deviceId : '',
              tokenBalanceRegistry: {},
              nativeBalanceRegistry: {},
              coin: info.coin,
              keyringId: info.keyringId
            }
          }
        )

        const selectedAccount = payload.selectedAccount
          ? accounts.find((account) => account.address.toLowerCase() === payload.selectedAccount.toLowerCase()) ?? accounts[0]
          : accounts[0]
        state.hasInitialized = true
        state.isWalletCreated = payload.isWalletCreated
        state.isFilecoinEnabled = payload.isFilecoinEnabled
        state.isSolanaEnabled = payload.isSolanaEnabled
        state.isWalletLocked = payload.isWalletLocked
        state.favoriteApps = payload.favoriteApps
        state.accounts = accounts
        state.isWalletBackedUp = payload.isWalletBackedUp
        state.selectedAccount = selectedAccount
        state.isNftPinningFeatureEnabled =
          payload.isNftPinningFeatureEnabled
        state.isPanelV2FeatureEnabled = payload.isPanelV2FeatureEnabled
      },

      nativeAssetBalancesUpdated (state: WalletState, { payload }: PayloadAction<GetNativeAssetBalancesPayload>) {
        state.accounts.forEach((account, accountIndex) => {
          payload.balances[accountIndex].forEach((info, tokenIndex) => {
            if (info.error === BraveWallet.ProviderError.kSuccess) {
              state.accounts[accountIndex].nativeBalanceRegistry[info.chainId] = Amount.normalize(info.balance)
            }
          })
        })

        // Refresh selectedAccount object
        const selectedAccount = state.accounts.find(
          account => account.address.toLowerCase() === state.selectedAccount?.address.toLowerCase()
        ) ?? state.selectedAccount

        state.selectedAccount = selectedAccount
      },

      newUnapprovedTxAdded (state: WalletState, { payload }: PayloadAction<NewUnapprovedTxAdded>) {
        state.pendingTransactions.push(payload.txInfo)
        if (state.pendingTransactions.length === 0) {
          state.selectedPendingTransaction = payload.txInfo
        }
      },

      portfolioPriceHistoryUpdated (state: WalletState, { payload }: PayloadAction<PortfolioTokenHistoryAndInfo[][]>) {
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
        const jointHistory = [].concat.apply([], [...history]).filter((h: []) => h.length > 1) as GetPriceHistoryReturnInfo[][]

        // Since the Price History API sometimes will return a shorter
        // array of history, this checks for the shortest array first to
        // then map and reduce to it length
        const shortestHistory = jointHistory.length > 0 ? jointHistory.reduce((a, b) => a.length <= b.length ? a : b) : []
        const sumOfHistory = jointHistory.length > 0 ? shortestHistory.map((token, tokenIndex) => {
          return {
            date: mojoTimeDeltaToJSDate(token.date),
            close: jointHistory.map(price => Number(price[tokenIndex].price) || 0).reduce((sum, x) => sum + x, 0)
          }
        }) : []

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

      setAccountTransactions (state: WalletState, { payload }: PayloadAction<AccountTransactions>) {
        const newPendingTransactions = state.accounts.map((account) => {
          return payload[account.address]
        }).flat(1)

        const filteredTransactions = newPendingTransactions?.filter((tx) => tx?.txStatus === BraveWallet.TransactionStatus.Unapproved) ?? []
        const sortedTransactionList = sortTransactionByDate(filteredTransactions)

        state.transactions = payload
        state.pendingTransactions = sortedTransactionList
        state.selectedPendingTransaction = sortedTransactionList[0]
      },

      setAllTokensList (state: WalletState, { payload }: PayloadAction<BraveWallet.BlockchainToken[]>) {
        state.fullTokenList = payload
      },

      setAssetAutoDiscoveryCompleted (state: WalletState, { payload }: PayloadAction<BraveWallet.BlockchainToken[]>) {
        state.assetAutoDiscoveryCompleted = true
      },

      setCoinMarkets (state: WalletState, { payload }: PayloadAction<GetCoinMarketsResponse>) {
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

      setDefaultAccounts (state: WalletState, { payload }: PayloadAction<BraveWallet.AccountInfo[]>) {
        state.defaultAccounts = payload
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

      setTransactionProviderError (state: WalletState, { payload }: PayloadAction<SetTransactionProviderErrorType>) {
        state.transactionProviderErrorRegistry[payload.transaction.id] = payload.providerError
      },

      setVisibleTokensInfo (state: WalletState, { payload }: PayloadAction<BraveWallet.BlockchainToken[]>) {
        state.userVisibleTokensInfo = payload
      },

      tokenBalancesUpdated (state: WalletState, { payload }: PayloadAction<GetBlockchainTokenBalanceReturnInfo>) {
        const visibleTokens = state.userVisibleTokensInfo
          .filter(asset => asset.contractAddress !== '')

        state.accounts.forEach((account, accountIndex) => {
          payload.balances[accountIndex]?.forEach((info, tokenIndex) => {
            if (info.error === BraveWallet.ProviderError.kSuccess) {
              const token = visibleTokens[tokenIndex]
              const registryKey = createTokenBalanceRegistryKey(token)
              state.accounts[accountIndex].tokenBalanceRegistry[registryKey] = Amount.normalize(info.balance)
            }
          })
        })

        // Refresh selectedAccount object
        const selectedAccount = state.accounts.find(
          account => account.address.toLowerCase() === state.selectedAccount?.address.toLowerCase()
        ) ?? state.selectedAccount

        state.selectedAccount = selectedAccount
      },

      unapprovedTxUpdated (state: WalletState, { payload }: PayloadAction<UnapprovedTxUpdated>) {
        const index = state.pendingTransactions.findIndex(
          (tx) => tx.id === payload.txInfo.id
        )

        if (index !== -1) {
          state.pendingTransactions[index] = payload.txInfo
        }

        if (state.selectedPendingTransaction?.id === payload.txInfo.id) {
          state.selectedPendingTransaction = payload.txInfo
        }
      },

      queueNextTransaction (state: WalletState) {
        const pendingTransactions = state.pendingTransactions

        const index = pendingTransactions.findIndex(
          (tx) => tx.id === state.selectedPendingTransaction?.id
        ) + 1

        state.selectedPendingTransaction = pendingTransactions.length === index
          ? pendingTransactions[0]
          : pendingTransactions[index]
      },

      refreshAccountInfo (state: WalletState, { payload }: PayloadAction<WalletInfoBase>) {
        state.accounts.forEach(account => {
          const info = payload.accountInfos.find(info => account.address === info.address)
          if (info) {
            account.name = info.name
          }
        })
      }
    },
    extraReducers (builder) {
      builder.addCase(WalletAsyncActions.locked.type, (state) => {
        state.isWalletLocked = true
      })

      builder.addCase(WalletAsyncActions.setSelectedAccountFilterItem, (state, { payload }) => {
        state.isFetchingPortfolioPriceHistory = true
        state.selectedAccountFilter = payload
      })

      builder.addCase(WalletAsyncActions.setSelectedNetworkFilter, (state, { payload }) => {
        state.isFetchingPortfolioPriceHistory = true
        state.selectedNetworkFilter = payload
      })

      builder.addCase(WalletAsyncActions.transactionStatusChanged, (state, { payload }) => {
        const newPendingTransactions = state.pendingTransactions
          .filter((tx) => tx.id !== payload.txInfo.id)
          .concat(payload.txInfo.txStatus === BraveWallet.TransactionStatus.Unapproved ? [payload.txInfo] : [])

        const sortedTransactionList = sortTransactionByDate(newPendingTransactions)

        const newTransactionEntries = Object.entries(state.transactions).map(([address, transactions]) => {
          const hasTransaction = transactions.some(tx => tx.id === payload.txInfo.id)

          return [
            address,
            hasTransaction
              ? sortTransactionByDate([
                ...transactions.filter(tx => tx.id !== payload.txInfo.id),
                payload.txInfo
              ])
              : transactions
          ]
        })

        state.pendingTransactions = sortedTransactionList
        state.selectedPendingTransaction = sortedTransactionList[0]
        state.transactions = Object.fromEntries(newTransactionEntries)
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
