/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  AccountInfo,
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
  SolFeeEstimates
} from '../../constants/types'
import {
  IsEip1559Changed,
  NewUnapprovedTxAdded,
  SetTransactionProviderErrorType,
  SitePermissionsPayloadType,
  TransactionStatusChanged,
  UnapprovedTxUpdated
} from '../constants/action_types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { mojoTimeDeltaToJSDate } from '../../../common/mojomUtils'
import { sortTransactionByDate } from '../../utils/tx-utils'
import Amount from '../../utils/amount'
import { AllNetworksOption } from '../../options/network-filter-options'

const defaultState: WalletState = {
  hasInitialized: false,
  isFilecoinEnabled: false,
  isSolanaEnabled: false,
  isTestNetworksEnabled: true,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  selectedNetwork: {
    chainId: BraveWallet.MAINNET_CHAIN_ID,
    chainName: 'Ethereum Mainnet',
    rpcUrls: [],
    blockExplorerUrls: [],
    iconUrls: [],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18,
    coin: BraveWallet.CoinType.ETH,
    data: {
      ethData: {
        isEip1559: true
      }
    }
  } as BraveWallet.NetworkInfo,
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
  networkList: [],
  transactionSpotPrices: [],
  addUserAssetError: false,
  defaultWallet: BraveWallet.DefaultWallet.BraveWalletPreferExtension,
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
  selectedCoin: BraveWallet.CoinType.ETH,
  defaultCurrencies: {
    fiat: '',
    crypto: ''
  },
  transactionProviderErrorRegistry: {},
  defaultNetworks: [] as BraveWallet.NetworkInfo[],
  defaultAccounts: [] as BraveWallet.AccountInfo[],
  selectedNetworkFilter: AllNetworksOption,
  solFeeEstimates: undefined
}

const getAccountType = (info: AccountInfo) => {
  if (info.hardware) {
    return info.hardware.vendor
  }
  return info.isImported ? 'Secondary' : 'Primary'
}

export const createWalletReducer = (initialState: WalletState) => {
  const reducer = createReducer<WalletState>({}, initialState)

  reducer.on(WalletActions.initialized, (state: WalletState, payload: WalletInfo): WalletState => {
    const accounts = payload.accountInfos.map((info: AccountInfo, idx: number) => {
      return {
        id: `${idx + 1}`,
        name: info.name,
        address: info.address,
        accountType: getAccountType(info),
        deviceId: info.hardware ? info.hardware.deviceId : '',
        tokenBalanceRegistry: {},
        nativeBalanceRegistry: {},
        coin: info.coin
      } as WalletAccountType
    })
    const selectedAccount = payload.selectedAccount
      ? accounts.find((account) => account.address.toLowerCase() === payload.selectedAccount.toLowerCase()) ?? accounts[0]
      : accounts[0]
    return {
      ...state,
      hasInitialized: true,
      isWalletCreated: payload.isWalletCreated,
      isFilecoinEnabled: payload.isFilecoinEnabled,
      isSolanaEnabled: payload.isSolanaEnabled,
      isWalletLocked: payload.isWalletLocked,
      favoriteApps: payload.favoriteApps,
      accounts,
      isWalletBackedUp: payload.isWalletBackedUp,
      selectedAccount
    }
  })

  reducer.on(WalletActions.hasIncorrectPassword, (state: WalletState, payload: boolean): WalletState => {
    return {
      ...state,
      hasIncorrectPassword: payload
    }
  })

  reducer.on(WalletActions.setSelectedAccount, (state: WalletState, payload: WalletAccountType): WalletState => {
    return {
      ...state,
      selectedAccount: payload
    }
  })

  reducer.on(WalletActions.setNetwork, (state: WalletState, payload: BraveWallet.NetworkInfo): WalletState => {
    return {
      ...state,
      selectedNetwork: payload
    }
  })

  reducer.on(WalletActions.setVisibleTokensInfo, (state: WalletState, payload: BraveWallet.BlockchainToken[]): WalletState => {
    return {
      ...state,
      userVisibleTokensInfo: payload
    }
  })

  reducer.on(WalletActions.setAllNetworks, (state: WalletState, payload: BraveWallet.NetworkInfo[]): WalletState => {
    return {
      ...state,
      networkList: payload
    }
  })

  reducer.on(WalletActions.setAllTokensList, (state: WalletState, payload: BraveWallet.BlockchainToken[]): WalletState => {
    return {
      ...state,
      fullTokenList: payload
    }
  })

  reducer.on(WalletActions.nativeAssetBalancesUpdated, (state: WalletState, payload: GetNativeAssetBalancesPayload): WalletState => {
    let accounts: WalletAccountType[] = [...state.accounts]

    accounts.forEach((account, accountIndex) => {
      payload.balances[accountIndex].forEach((info, tokenIndex) => {
        if (info.error === BraveWallet.ProviderError.kSuccess) {
          accounts[accountIndex].nativeBalanceRegistry[info.chainId] = Amount.normalize(info.balance)
        }
      })
    })

    // Refresh selectedAccount object
    const selectedAccount = accounts.find(
      account => account === state.selectedAccount
    ) ?? state.selectedAccount

    return {
      ...state,
      accounts,
      selectedAccount
    }
  })

  reducer.on(WalletActions.tokenBalancesUpdated, (state: WalletState, payload: GetBlockchainTokenBalanceReturnInfo): WalletState => {
    const visibleTokens = state.userVisibleTokensInfo
      .filter(asset => asset.contractAddress !== '')

    let accounts: WalletAccountType[] = [...state.accounts]
    accounts.forEach((account, accountIndex) => {
      payload.balances[accountIndex].forEach((info, tokenIndex) => {
        if (info.error === BraveWallet.ProviderError.kSuccess) {
          const contractAddress = visibleTokens[tokenIndex].contractAddress.toLowerCase()
          accounts[accountIndex].tokenBalanceRegistry[contractAddress] = Amount.normalize(info.balance)
        }
      })
    })

    // Refresh selectedAccount object
    const selectedAccount = accounts.find(
      account => account === state.selectedAccount
    ) ?? state.selectedAccount

    return {
      ...state,
      accounts,
      selectedAccount
    }
  })

  reducer.on(WalletActions.pricesUpdated, (state: WalletState, payload: GetPriceReturnInfo): WalletState => {
    return {
      ...state,
      transactionSpotPrices: payload.success ? payload.values : state.transactionSpotPrices
    }
  })

  reducer.on(WalletActions.portfolioPriceHistoryUpdated, (state: WalletState, payload: PortfolioTokenHistoryAndInfo[][]): WalletState => {
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

    return {
      ...state,
      portfolioPriceHistory: sumOfHistory,
      isFetchingPortfolioPriceHistory: sumOfHistory.length === 0
    }
  })

  reducer.on(WalletActions.portfolioTimelineUpdated, (state: WalletState, payload: BraveWallet.AssetPriceTimeframe): WalletState => {
    return {
      ...state,
      isFetchingPortfolioPriceHistory: true,
      selectedPortfolioTimeline: payload
    }
  })

  reducer.on(WalletActions.newUnapprovedTxAdded, (state: WalletState, payload: NewUnapprovedTxAdded): WalletState => {
    const newState = {
      ...state,
      pendingTransactions: [
        ...state.pendingTransactions,
        payload.txInfo
      ]
    }

    if (state.pendingTransactions.length === 0) {
      newState.selectedPendingTransaction = payload.txInfo
    }

    return newState
  })

  reducer.on(WalletActions.unapprovedTxUpdated, (state: WalletState, payload: UnapprovedTxUpdated): WalletState => {
    const newState = { ...state }

    const index = state.pendingTransactions.findIndex(
      (tx: BraveWallet.TransactionInfo) => tx.id === payload.txInfo.id
    )

    if (index !== -1) {
      newState.pendingTransactions[index] = payload.txInfo
    }

    if (state.selectedPendingTransaction?.id === payload.txInfo.id) {
      newState.selectedPendingTransaction = payload.txInfo
    }

    return newState
  })

  reducer.on(WalletActions.transactionStatusChanged, (state: WalletState, payload: TransactionStatusChanged): WalletState => {
    const newPendingTransactions = state.pendingTransactions
      .filter((tx: BraveWallet.TransactionInfo) => tx.id !== payload.txInfo.id)
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

    return {
      ...state,
      pendingTransactions: sortedTransactionList,
      selectedPendingTransaction: sortedTransactionList[0],
      transactions: Object.fromEntries(newTransactionEntries)
    }
  })

  reducer.on(WalletActions.setAccountTransactions, (state: WalletState, payload: AccountTransactions): WalletState => {
    const { accounts } = state

    const newPendingTransactions = accounts.map((account) => {
      return payload[account.address]
    }).flat(1)

    const filteredTransactions = newPendingTransactions?.filter((tx: BraveWallet.TransactionInfo) => tx?.txStatus === BraveWallet.TransactionStatus.Unapproved) ?? []

    const sortedTransactionList = sortTransactionByDate(filteredTransactions)

    return {
      ...state,
      transactions: payload,
      pendingTransactions: sortedTransactionList,
      selectedPendingTransaction: sortedTransactionList[0]
    }
  })

  reducer.on(WalletActions.addUserAssetError, (state: WalletState, payload: boolean): WalletState => {
    return {
      ...state,
      addUserAssetError: payload
    }
  })

  reducer.on(WalletActions.defaultWalletUpdated, (state: WalletState, payload: BraveWallet.DefaultWallet): WalletState => {
    return {
      ...state,
      defaultWallet: payload
    }
  })

  reducer.on(WalletActions.activeOriginChanged, (state: WalletState, payload: BraveWallet.OriginInfo): WalletState => {
    return {
      ...state,
      activeOrigin: payload
    }
  })

  reducer.on(WalletActions.isEip1559Changed, (state: WalletState, payload: IsEip1559Changed): WalletState => {
    const selectedNetwork = state.networkList.find(
      network => network.chainId === payload.chainId
    ) || state.selectedNetwork

    const updatedNetwork: BraveWallet.NetworkInfo = {
      ...selectedNetwork,
      data: {
        ethData: {
          isEip1559: payload.isEip1559
        }
      }
    }

    return {
      ...state,
      selectedNetwork: updatedNetwork,
      networkList: state.networkList.map(
        network => network.chainId === payload.chainId ? updatedNetwork : network
      )
    }
  })

  reducer.on(WalletActions.setGasEstimates, (state: WalletState, payload: BraveWallet.GasEstimation1559): WalletState => {
    return {
      ...state,
      gasEstimates: payload
    }
  })

  reducer.on(WalletActions.setSolFeeEstimates, (state: WalletState, payload: SolFeeEstimates): WalletState => {
    return {
      ...state,
      solFeeEstimates: payload
    }
  })

  reducer.on(WalletActions.setSitePermissions, (state: WalletState, payload: SitePermissionsPayloadType): WalletState => {
    return {
      ...state,
      connectedAccounts: payload.accounts
    }
  })

  reducer.on(WalletActions.queueNextTransaction, (state: WalletState): WalletState => {
    const pendingTransactions = state.pendingTransactions

    const index = pendingTransactions.findIndex(
      (tx: BraveWallet.TransactionInfo) => tx.id === state.selectedPendingTransaction?.id
    ) + 1

    let newPendingTransaction = pendingTransactions[index]
    if (pendingTransactions.length === index) {
      newPendingTransaction = pendingTransactions[0]
    }
    return {
      ...state,
      selectedPendingTransaction: newPendingTransaction
    }
  })

  reducer.on(WalletActions.setMetaMaskInstalled, (state: WalletState, payload: boolean): WalletState => {
    return {
      ...state,
      isMetaMaskInstalled: payload
    }
  })

  reducer.on(WalletActions.refreshAccountInfo, (state: WalletState, payload: WalletInfoBase): WalletState => {
    const accounts = state.accounts

    const updatedAccounts = accounts.map(account => {
      const info = payload.accountInfos.find(info => account.address === info.address)
      if (info) {
        account.name = info.name
      }
      return account
    })

    return {
      ...state,
      accounts: updatedAccounts
    }
  })

  reducer.on(WalletActions.defaultCurrenciesUpdated, (state: WalletState, payload: DefaultCurrencies): WalletState => {
    return {
      ...state,
      defaultCurrencies: payload
    }
  })

  reducer.on(WalletActions.setTransactionProviderError, (state: WalletState, payload: SetTransactionProviderErrorType): WalletState => {
    return {
      ...state,
      transactionProviderErrorRegistry: {
        ...state.transactionProviderErrorRegistry,
        [payload.transaction.id]: payload.providerError
      }
    }
  })

  reducer.on(WalletActions.setSelectedCoin, (state: WalletState, payload: BraveWallet.CoinType): WalletState => {
    return {
      ...state,
      selectedCoin: payload
    }
  })

  reducer.on(WalletActions.setDefaultNetworks, (state: WalletState, payload: BraveWallet.NetworkInfo[]): WalletState => {
    return {
      ...state,
      defaultNetworks: payload
    }
  })

  reducer.on(WalletActions.setDefaultAccounts, (state: WalletState, payload: BraveWallet.AccountInfo[]): WalletState => {
    return {
      ...state,
      defaultAccounts: payload
    }
  })

  reducer.on(WalletActions.setSelectedNetworkFilter, (state: WalletState, payload: BraveWallet.NetworkInfo): WalletState => {
    return {
      ...state,
      isFetchingPortfolioPriceHistory: true,
      selectedNetworkFilter: payload
    }
  })

  return reducer
}

const reducer = createWalletReducer(defaultState)

export default reducer
