/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global window */

import { createReducer } from 'redux-act'
import {
  WalletAccountType,
  WalletState,
  GetAllTokensReturnInfo,
  GetAllNetworksList,
  TokenInfo,
  GetETHBalancesPriceReturnInfo,
  GetERC20TokenBalanceAndPriceReturnInfo,
  AccountInfo,
  PortfolioTokenHistoryAndInfo,
  GetPriceHistoryReturnInfo,
  AssetPriceTimeframe,
  EthereumChain,
  kMainnetChainId,
  TransactionInfo,
  TransactionStatus
} from '../../constants/types'
import {
  NewUnapprovedTxAdded,
  TransactionStatusChanged,
  InitializedPayloadType
} from '../constants/action_types'
import { convertMojoTimeToJS } from '../../utils/mojo-time'
import * as WalletActions from '../actions/wallet_actions'
import { formatFiatBalance } from '../../utils/format-balances'
import { ETHIconUrl } from '../../assets/asset-icons'

const defaultState: WalletState = {
  hasInitialized: false,
  isWalletCreated: false,
  isWalletLocked: true,
  favoriteApps: [],
  isWalletBackedUp: false,
  hasIncorrectPassword: false,
  selectedAccount: {} as WalletAccountType,
  selectedNetwork: {
    chainId: kMainnetChainId,
    chainName: 'Ethereum Mainnet',
    rpcUrls: [],
    blockExplorerUrls: [],
    iconUrls: [],
    symbol: 'ETH',
    symbolName: 'Ethereum',
    decimals: 18
  } as EthereumChain,
  accounts: [],
  userVisibleTokens: [],
  userVisibleTokensInfo: [],
  transactions: [],
  pendingTransactions: [],
  knownTransactions: [],
  fullTokenList: [],
  portfolioPriceHistory: [],
  selectedPendingTransaction: undefined,
  isFetchingPortfolioPriceHistory: true,
  selectedPortfolioTimeline: AssetPriceTimeframe.OneDay,
  networkList: [],
  transactionSpotPrices: []
}

const reducer = createReducer<WalletState>({}, defaultState)

reducer.on(WalletActions.initialized, (state: any, payload: InitializedPayloadType) => {
  const accounts = payload.accountInfos.map((info: AccountInfo, idx: number) => {
    return {
      id: `${idx + 1}`,
      name: info.name,
      address: info.address,
      balance: '0',
      fiatBalance: '0',
      asset: 'eth',
      accountType: info.isImported ? 'Secondary' : 'Primary',
      tokens: []
    }
  })
  // VisibleTokens needs to be persited in prefs and returned in
  // in the initialized payload to be set here.
  return {
    ...state,
    hasInitialized: true,
    isWalletCreated: payload.isWalletCreated,
    isWalletLocked: payload.isWalletLocked,
    favoriteApps: payload.favoriteApps,
    accounts,
    isWalletBackedUp: payload.isWalletBackedUp,
    selectedAccount: accounts[0],
    userVisibleTokens: ['eth', '0x0D8775F648430679A709E98d2b0Cb6250d2887EF']
  }
})

reducer.on(WalletActions.hasIncorrectPassword, (state: any, payload: boolean) => {
  return {
    ...state,
    hasIncorrectPassword: payload
  }
})

reducer.on(WalletActions.selectAccount, (state: any, payload: WalletAccountType) => {
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

reducer.on(WalletActions.setVisibleTokensInfo, (state: any, payload: TokenInfo[]) => {
  const eth = {
    contractAddress: 'eth',
    name: 'Ethereum',
    isErc20: true,
    isErc721: false,
    symbol: 'ETH',
    decimals: 18,
    icon: ETHIconUrl
  }
  const list = [eth, ...payload]
  return {
    ...state,
    userVisibleTokensInfo: list
  }
})

reducer.on(WalletActions.setVisibleTokens, (state: any, payload: string[]) => {
  return {
    ...state,
    userVisibleTokens: payload
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

reducer.on(WalletActions.ethBalancesUpdated, (state: any, payload: GetETHBalancesPriceReturnInfo) => {
  let accounts: WalletAccountType[] = [...state.accounts]

  accounts.forEach((account, index) => {
    if (payload.balances[index].success) {
      accounts[index].balance = payload.balances[index].balance
      accounts[index].fiatBalance = formatFiatBalance(payload.balances[index].balance, 18, payload.usdPrice).toString()
    }
  })

  return {
    ...state,
    accounts
  }
})

reducer.on(WalletActions.tokenBalancesUpdated, (state: any, payload: GetERC20TokenBalanceAndPriceReturnInfo) => {
  const userVisibleTokensInfo: TokenInfo[] = state.userVisibleTokensInfo
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

      if (userVisibleTokensInfo[tokenIndex].contractAddress === 'eth') {
        assetBalance = account.balance
        fiatBalance = account.fiatBalance
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
      if (Number(token.token.assetBalance) !== 0) {
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
      date: convertMojoTimeToJS(token.date),
      close: jointHistory.map(price => Number(price[tokenIndex].price) || 0).reduce((sum, x) => sum + x, 0)
    }
  }) : []

  return {
    ...state,
    portfolioPriceHistory: sumOfHistory,
    isFetchingPortfolioPriceHistory: sumOfHistory.length === 0 ? true : false
  }
})

reducer.on(WalletActions.portfolioTimelineUpdated, (state: any, payload: AssetPriceTimeframe) => {
  return {
    ...state,
    isFetchingPortfolioPriceHistory: true,
    selectedPortfolioTimeline: payload
  }
})

reducer.on(WalletActions.newUnapprovedTxAdded, (state: any, payload: NewUnapprovedTxAdded) => {
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

reducer.on(WalletActions.transactionStatusChanged, (state: any, payload: TransactionStatusChanged) => {
  const newPendingTransactions =
    state.pendingTransactions.filter((tx: TransactionInfo) => tx.id !== payload.txInfo.id)
  const newSelectedPendingTransaction = newPendingTransactions.pop()
  if (payload.txInfo.txStatus === TransactionStatus.Submitted ||
    payload.txInfo.txStatus === TransactionStatus.Rejected ||
    payload.txInfo.txStatus === TransactionStatus.Approved) {
    const newState = {
      ...state,
      pendingTransactions: newPendingTransactions,
      selectedPendingTransaction: newSelectedPendingTransaction
    }
    return newState
  }
  return state
})

reducer.on(WalletActions.knownTransactionsUpdated, (state: any, payload: TransactionInfo[]) => {
  const newPendingTransactions =
    payload.filter((tx: TransactionInfo) => tx.txStatus === TransactionStatus.Unapproved)
  const newSelectedPendingTransaction = state.selectedPendingTransaction || newPendingTransactions.pop()
  return {
    ...state,
    pendingTransactions: newPendingTransactions,
    selectedPendingTransaction: newSelectedPendingTransaction,
    knownTransactions: payload
  }
})

export default reducer
