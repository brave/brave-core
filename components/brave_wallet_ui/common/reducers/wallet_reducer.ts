/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createReducer } from 'redux-act'
import {
  AccountInfo,
  AccountTransactions,
  AssetPriceTimeframe,
  DefaultWallet,
  EthereumChain,
  GasEstimation1559,
  GetAllNetworksList,
  GetAllTokensReturnInfo,
  GetERC20TokenBalanceAndPriceReturnInfo,
  GetNativeAssetBalancesPriceReturnInfo,
  GetPriceHistoryReturnInfo,
  MAINNET_CHAIN_ID,
  PortfolioTokenHistoryAndInfo,
  ERCToken,
  TransactionInfo,
  TransactionStatus,
  WalletAccountType,
  WalletState,
  WalletInfoBase,
  WalletInfo
} from '../../constants/types'
import {
  ActiveOriginChanged,
  IsEip1559Changed,
  NewUnapprovedTxAdded,
  SitePermissionsPayloadType,
  TransactionStatusChanged,
  UnapprovedTxUpdated
} from '../constants/action_types'
import { mojoTimeDeltaToJSDate } from '../../utils/datetime-utils'
import * as WalletActions from '../actions/wallet_actions'
import { formatFiatBalance } from '../../utils/format-balances'
import { sortTransactionByDate } from '../../utils/tx-utils'

const defaultState: WalletState = {
  hasInitialized: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  selectedNetwork: {
    chainId: MAINNET_CHAIN_ID,
    chainName: 'Ethereum Mainnet',
    rpcUrls: [],
    blockExplorerUrls: [],
    iconUrls: [],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18,
    isEip1559: true
  } as EthereumChain,
  accounts: [],
  userVisibleTokensInfo: [],
  transactions: {},
  pendingTransactions: [],
  knownTransactions: [],
  fullTokenList: [],
  portfolioPriceHistory: [],
  selectedPendingTransaction: undefined,
  isFetchingPortfolioPriceHistory: true,
  selectedPortfolioTimeline: AssetPriceTimeframe.OneDay,
  networkList: [],
  transactionSpotPrices: [],
  addUserAssetError: false,
  defaultWallet: DefaultWallet.BraveWalletPreferExtension,
  activeOrigin: '',
  gasEstimates: undefined,
  connectedAccounts: [],
  isMetaMaskInstalled: false
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
      fiatBalance: '',
      asset: 'eth',
      accountType: getAccountType(info),
      deviceId: info.hardware ? info.hardware.deviceId : '',
      tokens: []
    }
  })
  const selectedAccount = payload.selectedAccount
    ? accounts.find((account) => account.address.toLowerCase() === payload.selectedAccount.toLowerCase()) ?? accounts[0]
    : accounts[0]
  return {
    ...state,
    hasInitialized: true,
    isWalletCreated: payload.isWalletCreated,
    isWalletLocked: payload.isWalletLocked,
    favoriteApps: payload.favoriteApps,
    accounts,
    isWalletBackedUp: payload.isWalletBackedUp,
    selectedAccount: selectedAccount
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

reducer.on(WalletActions.setNetwork, (state: any, payload: EthereumChain) => {
  return {
    ...state,
    isFetchingPortfolioPriceHistory: true,
    selectedNetwork: payload
  }
})

reducer.on(WalletActions.setVisibleTokensInfo, (state: any, payload: ERCToken[]) => {
  return {
    ...state,
    userVisibleTokensInfo: payload
  }
})

reducer.on(WalletActions.setAllNetworks, (state: any, payload: GetAllNetworksList) => {
  return {
    ...state,
    networkList: payload.networks
  }
})

reducer.on(WalletActions.setAllTokensList, (state: any, payload: GetAllTokensReturnInfo) => {
  return {
    ...state,
    fullTokenList: payload.tokens
  }
})

reducer.on(WalletActions.nativeAssetBalancesUpdated, (state: any, payload: GetNativeAssetBalancesPriceReturnInfo) => {
  let accounts: WalletAccountType[] = [...state.accounts]

  accounts.forEach((account, index) => {
    if (payload.balances[index].success) {
      accounts[index].balance = payload.balances[index].balance
      accounts[index].fiatBalance = formatFiatBalance(payload.balances[index].balance, state.selectedNetwork.decimals, payload.usdPrice).toString()
    }
  })

  return {
    ...state,
    accounts
  }
})

reducer.on(WalletActions.tokenBalancesUpdated, (state: any, payload: GetERC20TokenBalanceAndPriceReturnInfo) => {
  const userTokens: ERCToken[] = state.userVisibleTokensInfo
  const userVisibleTokensInfo = userTokens.map((token) => {
    return {
      ...token,
      logo: `chrome://erc-token-images/${token.logo}`
    }
  })
  const prices = payload.prices
  const findTokenPrice = (symbol: string) => {
    if (prices.success) {
      return prices.values.find((value) => value.fromAsset === symbol.toLowerCase())?.price ?? '0'
    } else {
      return '0'
    }
  }
  let accounts: WalletAccountType[] = [...state.accounts]
  accounts.forEach((account, accountIndex) => {
    payload.balances[accountIndex].forEach((info, tokenIndex) => {
      let assetBalance = '0'
      let fiatBalance = '0'

      if (userVisibleTokensInfo[tokenIndex].contractAddress === '') {
        assetBalance = account.balance
        fiatBalance = account.fiatBalance
      } else if (info.success && userVisibleTokensInfo[tokenIndex].isErc721) {
        assetBalance = info.balance
        fiatBalance = '0' // TODO: support estimated market value.
      } else if (info.success) {
        assetBalance = info.balance
        fiatBalance = formatFiatBalance(info.balance, userVisibleTokensInfo[tokenIndex].decimals, findTokenPrice(userVisibleTokensInfo[tokenIndex].symbol))
      } else if (account.tokens[tokenIndex]) {
        assetBalance = account.tokens[tokenIndex].assetBalance
        fiatBalance = account.tokens[tokenIndex].fiatBalance
      }
      account.tokens.splice(tokenIndex, 1, {
        asset: userVisibleTokensInfo[tokenIndex],
        assetBalance,
        fiatBalance
      })
    })
  })
  return {
    ...state,
    transactionSpotPrices: prices.values,
    accounts
  }
})

reducer.on(WalletActions.portfolioPriceHistoryUpdated, (state: any, payload: PortfolioTokenHistoryAndInfo[][]) => {
  const history = payload.map((account) => {
    return account.map((token) => {
      if (Number(token.token.assetBalance) !== 0 && token.token.asset.visible) {
        return token.history.values.map((value) => {
          return {
            date: value.date,
            price: Number(formatFiatBalance(token.token.assetBalance, token.token.asset.decimals, value.price))
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

reducer.on(WalletActions.portfolioTimelineUpdated, (state: any, payload: AssetPriceTimeframe) => {
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
    (tx: TransactionInfo) => tx.id === payload.txInfo.id)
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
    .filter((tx: TransactionInfo) => tx.id !== payload.txInfo.id)
    .concat(payload.txInfo.txStatus === TransactionStatus.Unapproved ? [payload.txInfo] : [])

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
    ? payload[selectedAccount.address].filter((tx: TransactionInfo) => tx.txStatus === TransactionStatus.Unapproved) : []

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

reducer.on(WalletActions.defaultWalletUpdated, (state: any, payload: DefaultWallet) => {
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

  const updatedNetwork: EthereumChain = {
    ...selectedNetwork,
    isEip1559: payload.isEip1559
  }

  return {
    ...state,
    selectedNetwork: updatedNetwork,
    networkList: state.networkList.map(
      network => network.chainId === payload.chainId ? updatedNetwork : network
    )
  }
})

reducer.on(WalletActions.setGasEstimates, (state: any, payload: GasEstimation1559) => {
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
  const index = pendingTransactions.findIndex((tx: TransactionInfo) => tx.id === state.selectedPendingTransaction.id) + 1
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

export default reducer
