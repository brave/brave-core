/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  AccountInfo,
  AccountTransactions,
  BraveWallet,
  GetAllNetworksList,
  GetAllTokensReturnInfo,
  GetBlockchainTokenBalanceReturnInfo,
  GetNativeAssetBalancesReturnInfo,
  GetPriceHistoryReturnInfo,
  PortfolioTokenHistoryAndInfo,
  WalletAccountType,
  WalletState,
  WalletInfoBase,
  WalletInfo,
  DefaultCurrencies,
  GetPriceReturnInfo
} from '../../constants/types'
import {
  ActiveOriginChanged,
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
  activeOrigin: '',
  gasEstimates: undefined,
  connectedAccounts: [],
  isMetaMaskInstalled: false,
  defaultCurrencies: {
    fiat: '',
    crypto: ''
  },
  transactionProviderErrorRegistry: {}
}

const reducer = createReducer<WalletState>({}, defaultState)

const getAccountType = (info: AccountInfo) => {
  if (info.hardware) {
    return info.hardware.vendor
  }
  return info.isImported ? 'Secondary' : 'Primary'
}

reducer.on(WalletActions.initialized, (state: any, payload: WalletInfo) => {
  const accounts = payload.accountInfos.map((info: AccountInfo, idx: number) => {
    return {
      id: `${idx + 1}`,
      name: info.name,
      address: info.address,
      balance: '',
      accountType: getAccountType(info),
      deviceId: info.hardware ? info.hardware.deviceId : '',
      tokenBalanceRegistry: {},
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

reducer.on(WalletActions.hasIncorrectPassword, (state: any, payload: boolean) => {
  return {
    ...state,
    hasIncorrectPassword: payload
  }
})

reducer.on(WalletActions.setSelectedAccount, (state: any, payload: WalletAccountType) => {
  return {
    ...state,
    selectedAccount: payload
  }
})

reducer.on(WalletActions.setNetwork, (state: any, payload: BraveWallet.NetworkInfo) => {
  return {
    ...state,
    isFetchingPortfolioPriceHistory: true,
    selectedNetwork: payload
  }
})

reducer.on(WalletActions.setVisibleTokensInfo, (state: WalletState, payload: BraveWallet.BlockchainToken[]) => {
  const userVisibleTokensInfo = payload.map((token) => ({
    ...token,
    logo: `chrome://erc-token-images/${token.logo}`
  })) as BraveWallet.BlockchainToken[]

  return {
    ...state,
    userVisibleTokensInfo
  }
})

reducer.on(WalletActions.setAllNetworks, (state: any, payload: GetAllNetworksList) => {
  return {
    ...state,
    networkList: payload.networks
  }
})

reducer.on(WalletActions.setAllTokensList, (state: WalletState, payload: GetAllTokensReturnInfo) => {
  return {
    ...state,
    fullTokenList: payload.tokens.map(token => ({
      ...token,
      logo: `chrome://erc-token-images/${token.logo}`
    }))
  }
})

reducer.on(WalletActions.nativeAssetBalancesUpdated, (state: WalletState, payload: GetNativeAssetBalancesReturnInfo) => {
  let accounts: WalletAccountType[] = [...state.accounts]

  accounts.forEach((account, index) => {
    if (payload.balances[index].error === BraveWallet.ProviderError.kSuccess) {
      accounts[index].balance = Amount.normalize(payload.balances[index].balance)
    }
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

reducer.on(WalletActions.tokenBalancesUpdated, (state: WalletState, payload: GetBlockchainTokenBalanceReturnInfo) => {
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

reducer.on(WalletActions.pricesUpdated, (state: WalletState, payload: GetPriceReturnInfo) => {
  return {
    ...state,
    transactionSpotPrices: payload.success ? payload.values : state.transactionSpotPrices
  }
})

reducer.on(WalletActions.portfolioPriceHistoryUpdated, (state: any, payload: PortfolioTokenHistoryAndInfo[][]) => {
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

reducer.on(WalletActions.portfolioTimelineUpdated, (state: any, payload: BraveWallet.AssetPriceTimeframe) => {
  return {
    ...state,
    isFetchingPortfolioPriceHistory: true,
    selectedPortfolioTimeline: payload
  }
})

reducer.on(WalletActions.newUnapprovedTxAdded, (state: WalletState, payload: NewUnapprovedTxAdded) => {
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

reducer.on(WalletActions.unapprovedTxUpdated, (state: any, payload: UnapprovedTxUpdated) => {
  const newState = { ...state }

  const index = state.pendingTransactions.findIndex(
    (tx: BraveWallet.TransactionInfo) => tx.id === payload.txInfo.id)
  if (index !== -1) {
    newState.pendingTransactions[index] = payload.txInfo
  }

  if (state.selectedPendingTransaction.id === payload.txInfo.id) {
    newState.selectedPendingTransaction = payload.txInfo
  }

  return newState
})

reducer.on(WalletActions.transactionStatusChanged, (state: WalletState, payload: TransactionStatusChanged) => {
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

reducer.on(WalletActions.setAccountTransactions, (state: WalletState, payload: AccountTransactions) => {
  const { selectedAccount } = state
  const newPendingTransactions = selectedAccount
    ? payload[selectedAccount.address].filter((tx: BraveWallet.TransactionInfo) => tx.txStatus === BraveWallet.TransactionStatus.Unapproved) : []

  const sortedTransactionList = sortTransactionByDate(newPendingTransactions)

  return {
    ...state,
    transactions: payload,
    pendingTransactions: sortedTransactionList,
    selectedPendingTransaction: sortedTransactionList[0]
  }
})

reducer.on(WalletActions.addUserAssetError, (state: any, payload: boolean) => {
  return {
    ...state,
    addUserAssetError: payload
  }
})

reducer.on(WalletActions.defaultWalletUpdated, (state: any, payload: BraveWallet.DefaultWallet) => {
  return {
    ...state,
    defaultWallet: payload
  }
})

reducer.on(WalletActions.activeOriginChanged, (state: any, payload: ActiveOriginChanged) => {
  return {
    ...state,
    activeOrigin: payload.origin
  }
})

reducer.on(WalletActions.isEip1559Changed, (state: WalletState, payload: IsEip1559Changed) => {
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

reducer.on(WalletActions.setGasEstimates, (state: any, payload: BraveWallet.GasEstimation1559) => {
  return {
    ...state,
    gasEstimates: payload
  }
})

reducer.on(WalletActions.setSitePermissions, (state: any, payload: SitePermissionsPayloadType) => {
  return {
    ...state,
    connectedAccounts: payload.accounts
  }
})

reducer.on(WalletActions.queueNextTransaction, (state: any) => {
  const pendingTransactions = state.pendingTransactions
  const index = pendingTransactions.findIndex((tx: BraveWallet.TransactionInfo) => tx.id === state.selectedPendingTransaction.id) + 1
  let newPendingTransaction = pendingTransactions[index]
  if (pendingTransactions.length === index) {
    newPendingTransaction = pendingTransactions[0]
  }
  return {
    ...state,
    selectedPendingTransaction: newPendingTransaction
  }
})

reducer.on(WalletActions.setMetaMaskInstalled, (state: WalletState, payload: boolean) => {
  return {
    ...state,
    isMetaMaskInstalled: payload
  }
})

reducer.on(WalletActions.refreshAccountInfo, (state: any, payload: WalletInfoBase) => {
  const accounts = state.accounts
  const updatedAccounts = payload.accountInfos.map((info: AccountInfo) => {
    let account = accounts.find((account: WalletAccountType) => account.address === info.address)
    account.name = info.name
    return account
  })
  return {
    ...state,
    accounts: updatedAccounts
  }
})

reducer.on(WalletActions.defaultCurrenciesUpdated, (state: any, payload: DefaultCurrencies) => {
  return {
    ...state,
    defaultCurrencies: payload
  }
})

reducer.on(WalletActions.setTransactionProviderError, (state: WalletState, payload: SetTransactionProviderErrorType) => {
  return {
    ...state,
    transactionProviderErrorRegistry: {
      ...state.transactionProviderErrorRegistry,
      [payload.transaction.id]: payload.providerError
    }
  }
})

export default reducer
