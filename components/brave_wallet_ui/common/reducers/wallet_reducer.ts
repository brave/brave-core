/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { createAction, createReducer } from 'redux-act'
import {
  AccountInfo,
  AccountTransactions,
  BraveWallet,
  GetBlockchainTokenBalanceReturnInfo,
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
  UnapprovedTxUpdated,
  BalancesAndPricesRefreshedPayload,
  WalletInfoUpdatedPayload,
  ChainChangeCompletedPayload
} from '../constants/action_types'
import * as WalletActions from '../actions/wallet_actions'

// Utils
import { sortTransactionByDate } from '../../utils/tx-utils'
import Amount from '../../utils/amount'
import { AllNetworksOption } from '../../options/network-filter-options'
import { getAccountType } from '../../utils/account-utils'

export const defaultWalletState: WalletState = {
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

export const createWalletReducer = (initialState: WalletState) => {
  const reducer = createReducer<WalletState>({}, initialState)

  reducer.on(WalletActions.initialized, (state: WalletState, payload: WalletInfo): WalletState => {
    alert(`r - ${WalletActions.initialized.getType()}`)
    console.log(`r - ${WalletActions.initialized.getType()}`)
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
      }
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
    alert(`r - ${WalletActions.hasIncorrectPassword.getType()}`)
    console.log(`r - ${WalletActions.hasIncorrectPassword.getType()}`)
    return {
      ...state,
      hasIncorrectPassword: payload
    }
  })

  reducer.on(WalletActions.setSelectedAccount, (state: WalletState, payload: WalletAccountType): WalletState => {
    alert(`r - ${WalletActions.setSelectedAccount.getType()}`)
    console.log(`r - ${WalletActions.setSelectedAccount.getType()}`)
    return {
      ...state,
      selectedAccount: payload
    }
  })

  reducer.on(WalletActions.setNetwork, (state: WalletState, payload: BraveWallet.NetworkInfo): WalletState => {
    alert(`r - ${WalletActions.setNetwork.getType()}`)
    console.log(`r - ${WalletActions.setNetwork.getType()}`)
    return {
      ...state,
      selectedNetwork: payload
    }
  })

  reducer.on(WalletActions.setVisibleTokensInfo, (state: WalletState, payload: BraveWallet.BlockchainToken[]): WalletState => {
    alert(`r - ${WalletActions.setVisibleTokensInfo.getType()}`)
    console.log(`r - ${WalletActions.setVisibleTokensInfo.getType()}`)
    return {
      ...state,
      userVisibleTokensInfo: payload
    }
  })

  reducer.on(WalletActions.setAllNetworks, (state: WalletState, payload: BraveWallet.NetworkInfo[]): WalletState => {
    alert(`r - ${WalletActions.setAllNetworks.getType()}`)
    console.log(`r - ${WalletActions.setAllNetworks.getType()}`)
    return {
      ...state,
      networkList: payload
    }
  })

  reducer.on(WalletActions.setAllTokensList, (state: WalletState, payload: BraveWallet.BlockchainToken[]): WalletState => {
    alert(`r - ${WalletActions.setAllTokensList.getType()}`)
    console.log(`r - ${WalletActions.setAllTokensList.getType()}`)
    return {
      ...state,
      fullTokenList: payload
    }
  })

  reducer.on(WalletActions.nativeAssetBalancesUpdated, (state: WalletState, payload: GetNativeAssetBalancesPayload): WalletState => {
    alert(`r - ${WalletActions.nativeAssetBalancesUpdated.getType()}`)
    console.log(`r - ${WalletActions.nativeAssetBalancesUpdated.getType()}`)
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
    alert(`r - ${WalletActions.tokenBalancesUpdated.getType()}`)
    console.log(`r - ${WalletActions.tokenBalancesUpdated.getType()}`)
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
    alert(`r - ${WalletActions.pricesUpdated.getType()}`)
    console.log(`r - ${WalletActions.pricesUpdated.getType()}`)
    return {
      ...state,
      transactionSpotPrices: payload.success ? payload.values : state.transactionSpotPrices
    }
  })

  reducer.on(WalletActions.portfolioPriceHistoryUpdated, (state, payload): WalletState => {
    alert(`r - ${WalletActions.portfolioPriceHistoryUpdated.getType()}`)
    console.log(`r - ${WalletActions.portfolioPriceHistoryUpdated.getType()}`)
    return {
      ...state,
      portfolioPriceHistory: payload.portfolioPriceHistory,
      isFetchingPortfolioPriceHistory: payload.isFetchingPortfolioPriceHistory
    }
  })

  reducer.on(WalletActions.portfolioTimelineUpdated, (state: WalletState, payload: BraveWallet.AssetPriceTimeframe): WalletState => {
    alert(`r - ${WalletActions.portfolioTimelineUpdated.getType()}`)
    console.log(`r - ${WalletActions.portfolioTimelineUpdated.getType()}`)
    return {
      ...state,
      isFetchingPortfolioPriceHistory: true,
      selectedPortfolioTimeline: payload
    }
  })

  reducer.on(WalletActions.newUnapprovedTxAdded, (state: WalletState, payload: NewUnapprovedTxAdded): WalletState => {
    alert(`r - ${WalletActions.newUnapprovedTxAdded.getType()}`)
    console.log(`r - ${WalletActions.newUnapprovedTxAdded.getType()}`)
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
    alert(`r - ${WalletActions.unapprovedTxUpdated.getType()}`)
    console.log(`r - ${WalletActions.unapprovedTxUpdated.getType()}`)
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
    alert(`r - ${WalletActions.transactionStatusChanged.getType()}`)
    console.log(`r - ${WalletActions.transactionStatusChanged.getType()}`)
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
    alert(`r - ${WalletActions.setAccountTransactions.getType()}`)
    console.log(`r - ${WalletActions.setAccountTransactions.getType()}`)
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
    alert(`r - ${WalletActions.addUserAssetError.getType()}`)
    console.log(`r - ${WalletActions.addUserAssetError.getType()}`)
    return {
      ...state,
      addUserAssetError: payload
    }
  })

  reducer.on(WalletActions.defaultWalletUpdated, (state: WalletState, payload: BraveWallet.DefaultWallet): WalletState => {
    alert(`r - ${WalletActions.defaultWalletUpdated.getType()}`)
    console.log(`r - ${WalletActions.defaultWalletUpdated.getType()}`)
    return {
      ...state,
      defaultWallet: payload
    }
  })

  reducer.on(WalletActions.activeOriginChanged, (state: WalletState, payload: BraveWallet.OriginInfo): WalletState => {
    console.log(`r - ${WalletActions.activeOriginChanged.getType()}`)
    return {
      ...state,
      activeOrigin: payload
    }
  })

  reducer.on(WalletActions.isEip1559Changed, (state: WalletState, payload: IsEip1559Changed): WalletState => {
    alert(`r - ${WalletActions.isEip1559Changed.getType()}`)
    console.log(`r - ${WalletActions.isEip1559Changed.getType()}`)
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
    alert(`r - ${WalletActions.setGasEstimates.getType()}`)
    console.log(`r - ${WalletActions.setGasEstimates.getType()}`)
    return {
      ...state,
      gasEstimates: payload
    }
  })

  reducer.on(WalletActions.setSolFeeEstimates, (state: WalletState, payload: SolFeeEstimates): WalletState => {
    alert(`r - ${WalletActions.setSolFeeEstimates.getType()}`)
    console.log(`r - ${WalletActions.setSolFeeEstimates.getType()}`)
    return {
      ...state,
      solFeeEstimates: payload
    }
  })

  reducer.on(WalletActions.setSitePermissions, (state: WalletState, payload: SitePermissionsPayloadType): WalletState => {
    alert(`r - ${WalletActions.setSitePermissions.getType()}`)
    console.log(`r - ${WalletActions.setSitePermissions.getType()}`)
    return {
      ...state,
      connectedAccounts: payload.accounts
    }
  })

  reducer.on(WalletActions.queueNextTransaction, (state: WalletState): WalletState => {
    alert(`r - ${WalletActions.queueNextTransaction.getType()}`)
    console.log(`r - ${WalletActions.queueNextTransaction.getType()}`)
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
    alert(`r - ${WalletActions.setMetaMaskInstalled.getType()}`)
    console.log(`r - ${WalletActions.setMetaMaskInstalled.getType()}`)
    return {
      ...state,
      isMetaMaskInstalled: payload
    }
  })

  reducer.on(WalletActions.refreshAccountInfo, (state: WalletState, payload: WalletInfoBase): WalletState => {
    alert(`r - ${WalletActions.refreshAccountInfo.getType()}`)
    console.log(`r - ${WalletActions.refreshAccountInfo.getType()}`)
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
    alert(`r - ${WalletActions.defaultCurrenciesUpdated.getType()}`)
    console.log(`r - ${WalletActions.defaultCurrenciesUpdated.getType()}`)
    return {
      ...state,
      defaultCurrencies: payload
    }
  })

  reducer.on(WalletActions.setTransactionProviderError, (state: WalletState, payload: SetTransactionProviderErrorType): WalletState => {
    alert(`r - ${WalletActions.setTransactionProviderError.getType()}`)
    console.log(`r - ${WalletActions.setTransactionProviderError.getType()}`)
    return {
      ...state,
      transactionProviderErrorRegistry: {
        ...state.transactionProviderErrorRegistry,
        [payload.transaction.id]: payload.providerError
      }
    }
  })

  reducer.on(WalletActions.setSelectedCoin, (state: WalletState, payload: BraveWallet.CoinType): WalletState => {
    alert(`r - ${WalletActions.setSelectedCoin.getType()}`)
    console.log(`r - ${WalletActions.setSelectedCoin.getType()}`)
    return {
      ...state,
      selectedCoin: payload
    }
  })

  reducer.on(WalletActions.setDefaultNetworks, (state: WalletState, payload: BraveWallet.NetworkInfo[]): WalletState => {
    alert(`r - ${WalletActions.setDefaultNetworks.getType()}`)
    console.log(`r - ${WalletActions.setDefaultNetworks.getType()}`)
    return {
      ...state,
      defaultNetworks: payload
    }
  })

  reducer.on(WalletActions.setDefaultAccounts, (state: WalletState, payload: BraveWallet.AccountInfo[]): WalletState => {
    alert(`r - ${WalletActions.setDefaultAccounts.getType()}`)
    console.log(`r - ${WalletActions.setDefaultAccounts.getType()}`)
    return {
      ...state,
      defaultAccounts: payload
    }
  })

  reducer.on(WalletActions.setSelectedNetworkFilter, (state: WalletState, payload: BraveWallet.NetworkInfo): WalletState => {
    alert(`r - ${WalletActions.setSelectedNetworkFilter.getType()}`)
    console.log(`r - ${WalletActions.setSelectedNetworkFilter.getType()}`)
    return {
      ...state,
      isFetchingPortfolioPriceHistory: true,
      selectedNetworkFilter: payload
    }
  })

  reducer.on(WalletActions.balancesAndPricesRefreshed, (state: WalletState, payload: BalancesAndPricesRefreshedPayload) => {
    alert(`r - ${WalletActions.balancesAndPricesRefreshed.getType()}`)
    console.log(`r - ${WalletActions.balancesAndPricesRefreshed.getType()}`)
    return {
      ...state,
      userVisibleTokensInfo: payload.userVisibleTokensInfo,
      accounts: payload.accounts,
      selectedAccount: payload.selectedAccount,
      transactionSpotPrices: payload.transactionSpotPrices
    }
  })

  reducer.on(WalletActions.walletInfoUpdated, (state: WalletState, payload: WalletInfoUpdatedPayload): WalletState => {
    alert(`r - ${WalletActions.walletInfoUpdated.getType()}`)
    console.log(`r - ${WalletActions.walletInfoUpdated.getType()}`)
    return {
      ...state,
      accounts: payload.accounts,
      connectedAccounts: payload?.connectedAccounts ?? state.connectedAccounts,
      defaultAccounts: payload.defaultAccounts,
      defaultNetworks: payload?.defaultNetworks ?? state.defaultNetworks,
      defaultWallet: payload?.defaultWallet ?? state.defaultWallet,
      favoriteApps: payload.favoriteApps,
      fullTokenList: payload?.fullTokenList ?? state.fullTokenList,
      isFilecoinEnabled: payload.isFilecoinEnabled,
      isMetaMaskInstalled: payload?.isMetaMaskInstalled ?? state.isMetaMaskInstalled,
      isSolanaEnabled: payload.isSolanaEnabled,
      isWalletBackedUp: payload.isWalletBackedUp,
      isWalletCreated: payload.isWalletCreated,
      isWalletLocked: payload.isWalletLocked,
      networkList: payload?.networkList ?? state.networkList,
      pendingTransactions: payload?.pendingTransactions ?? state.pendingTransactions,
      selectedAccount: payload.selectedAccount,
      selectedNetwork: payload?.selectedNetwork ?? state.selectedNetwork,
      selectedPendingTransaction: payload.selectedPendingTransaction,
      transactions: payload?.transactions ?? state.transactions
    }
  })

  reducer.on(WalletActions.accountsUpdated, (state, payload) => {
    alert(`r - ${WalletActions.accountsUpdated.getType()}`)
    console.log(`r - ${WalletActions.accountsUpdated.getType()}`)
    return {
      ...state,
      accounts: payload.accounts,
      selectedAccount: payload.selectedAccount
    }
  })

  reducer.on(WalletActions.chainChangeComplete, (state: WalletState, payload: ChainChangeCompletedPayload) => {
    alert(`r - ${WalletActions.chainChangeComplete.getType()}`)
    console.log(`r - ${WalletActions.chainChangeComplete.getType()}`)
    return {
      ...state,
      selectedCoin: payload.coin,
      selectedAccount: payload.selectedAccount
    }
  })

  reducer.on(WalletActions.walletDataInitialized, (state, payload) => {
    alert(`r - ${WalletActions.walletDataInitialized.getType()}`)
    console.log(`r - ${WalletActions.walletDataInitialized.getType()}`)
    return {
      ...state,
      ...payload,
      hasInitialized: true
    }
  })

  return reducer
}

const reducer = createWalletReducer(defaultWalletState)

export const setEmulatedWalletState = createAction<WalletState>('setEmulatedWalletState')
/**
 * use this to compute intermediate states without dispatching actions
 */
export const emulatedWalletReducer = createWalletReducer(defaultWalletState)
  .on(setEmulatedWalletState, (_state, payload: WalletState) => payload)

export const emulateWalletReducer = (state: WalletState) => {
  emulatedWalletReducer(state, setEmulatedWalletState(state))
  return emulateWalletReducer
}

export default reducer
