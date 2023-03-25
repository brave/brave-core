// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPageState } from '../../constants/types'

type State = Omit<WalletPageState, 'page'>

// safe selectors (primitive return types only)
export const addUserAssetError = ({ wallet }: State) => wallet.addUserAssetError
export const defaultEthereumWallet = ({ wallet }: State) => wallet.defaultEthereumWallet
export const defaultSolanaWallet = ({ wallet }: State) => wallet.defaultSolanaWallet
export const defaultCryptocurrency = ({ wallet }: State) => wallet.defaultCurrencies.crypto
export const defaultFiatCurrency = ({ wallet }: State) => wallet.defaultCurrencies.fiat
export const hasIncorrectPassword = ({ wallet }: State) => wallet.hasIncorrectPassword
export const hasInitialized = ({ wallet }: State) => wallet.hasInitialized
export const isFetchingPortfolioPriceHistory = ({ wallet }: State) => wallet.isFetchingPortfolioPriceHistory
export const isFilecoinEnabled = ({ wallet }: State) => wallet.isFilecoinEnabled
export const isLoadingCoinMarketData = ({ wallet }: State) => wallet.isLoadingCoinMarketData
export const isMetaMaskInstalled = ({ wallet }: State) => wallet.isMetaMaskInstalled
export const isSolanaEnabled = ({ wallet }: State) => wallet.isSolanaEnabled
export const isWalletBackedUp = ({ wallet }: State) => wallet.isWalletBackedUp
export const isWalletCreated = ({ wallet }: State) => wallet.isWalletCreated
export const isWalletLocked = ({ wallet }: State) => wallet.isWalletLocked
export const passwordAttempts = ({ wallet }: State) => wallet.passwordAttempts
export const selectedPortfolioTimeline = ({ wallet }: State) => wallet.selectedPortfolioTimeline
export const assetAutoDiscoveryCompleted = ({ wallet }: State) => wallet.assetAutoDiscoveryCompleted
export const hasFeeEstimatesError = ({ wallet }: State) =>
  wallet.hasFeeEstimatesError
export const isNftPinningFeatureEnabled = ({ wallet }: State) => wallet.isNftPinningFeatureEnabled

// unsafe selectors (will cause re-render if not strictly equal "===") (objects and lists)
export const accounts = ({ wallet }: State) => wallet.accounts
export const activeOrigin = ({ wallet }: State) => wallet.activeOrigin
export const coinMarketData = ({ wallet }: State) => wallet.coinMarketData
export const connectedAccounts = ({ wallet }: State) => wallet.connectedAccounts
export const defaultAccounts = ({ wallet }: State) => wallet.defaultAccounts
export const defaultCurrencies = ({ wallet }: State) => wallet.defaultCurrencies
export const favoriteApps = ({ wallet }: State) => wallet.favoriteApps
export const fullTokenList = ({ wallet }: State) => wallet.fullTokenList
export const gasEstimates = ({ wallet }: State) => wallet.gasEstimates
export const knownTransactions = ({ wallet }: State) => wallet.knownTransactions
export const onRampCurrencies = ({ wallet }: State) => wallet.onRampCurrencies
export const pendingTransactions = ({ wallet }: State) => wallet.pendingTransactions
export const portfolioPriceHistory = ({ wallet }: State) => wallet.portfolioPriceHistory
export const selectedAccount = ({ wallet }: State) => wallet.selectedAccount
export const selectedAssetFilter = ({ wallet }: State) => wallet.selectedAssetFilter
export const selectedCurrency = ({ wallet }: State) => wallet.selectedCurrency
export const selectedNetworkFilter = ({ wallet }: State) => wallet.selectedNetworkFilter
export const selectedPendingTransaction = ({ wallet }: State) => wallet.selectedPendingTransaction
export const solFeeEstimates = ({ wallet }: State) => wallet.solFeeEstimates
export const transactionProviderErrorRegistry = ({ wallet }: State) => wallet.transactionProviderErrorRegistry
export const transactionSpotPrices = ({ wallet }: State) => wallet.transactionSpotPrices
export const transactions = ({ wallet }: State) => wallet.transactions
export const userVisibleTokensInfo = ({ wallet }: State) => wallet.userVisibleTokensInfo
export const selectedAccountFilter = ({ wallet }: State) => wallet.selectedAccountFilter
