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
export const isFilecoinEnabled = ({ wallet }: State) => wallet.isFilecoinEnabled
export const isSolanaEnabled = ({ wallet }: State) => wallet.isSolanaEnabled
export const isBitcoinEnabled = ({ wallet }: State) => wallet.isBitcoinEnabled
export const isZCashEnabled = ({ wallet }: State) => wallet.isZCashEnabled
export const isLoadingCoinMarketData = ({ wallet }: State) => wallet.isLoadingCoinMarketData
export const isMetaMaskInstalled = ({ wallet }: State) => wallet.isMetaMaskInstalled
export const isWalletBackedUp = ({ wallet }: State) => wallet.isWalletBackedUp
export const isWalletCreated = ({ wallet }: State) => wallet.isWalletCreated
export const isWalletLocked = ({ wallet }: State) => wallet.isWalletLocked
export const passwordAttempts = ({ wallet }: State) => wallet.passwordAttempts
export const selectedPortfolioTimeline = ({ wallet }: State) => wallet.selectedPortfolioTimeline
export const assetAutoDiscoveryCompleted = ({ wallet }: State) => wallet.assetAutoDiscoveryCompleted
export const hasFeeEstimatesError = ({ wallet }: State) =>
  wallet.hasFeeEstimatesError
export const isNftPinningFeatureEnabled = ({ wallet }: State) => wallet.isNftPinningFeatureEnabled
export const hidePortfolioGraph = ({ wallet }: State) =>
  wallet.hidePortfolioGraph
export const hidePortfolioBalances = ({ wallet }: State) =>
  wallet.hidePortfolioBalances
export const hidePortfolioNFTsTab = ({ wallet }: State) =>
  wallet.hidePortfolioNFTsTab
export const removedNonFungibleTokens = ({ wallet }: State) => wallet.removedNonFungibleTokens
export const filteredOutPortfolioNetworkKeys = ({ wallet }: State) =>
  wallet.filteredOutPortfolioNetworkKeys
export const filteredOutPortfolioAccountAddresses = ({ wallet }: State) =>
  wallet.filteredOutPortfolioAccountAddresses
export const hidePortfolioSmallBalances = ({ wallet }: State) =>
  wallet.hidePortfolioSmallBalances
export const showNetworkLogoOnNfts = ({ wallet }: State) => wallet.showNetworkLogoOnNfts
export const isPanelV2FeatureEnabled = ({ wallet }: State) =>
  wallet.isPanelV2FeatureEnabled
export const isAnkrBalancesFeatureEnabled = ({ wallet }: State) =>
  wallet.isAnkrBalancesFeatureEnabled
export const importAccountError = ({ wallet }: State) =>
  wallet.importAccountError
export const selectedOnRampAssetId = ({ wallet }: State) =>
  wallet.selectedDepositAssetId

// unsafe selectors (will cause re-render if not strictly equal "===") (objects and lists)
export const activeOrigin = ({ wallet }: State) => wallet.activeOrigin
export const coinMarketData = ({ wallet }: State) => wallet.coinMarketData
export const connectedAccounts = ({ wallet }: State) => wallet.connectedAccounts
export const defaultCurrencies = ({ wallet }: State) => wallet.defaultCurrencies
export const favoriteApps = ({ wallet }: State) => wallet.favoriteApps
export const fullTokenList = ({ wallet }: State) => wallet.fullTokenList
export const gasEstimates = ({ wallet }: State) => wallet.gasEstimates
export const selectedAssetFilter = ({ wallet }: State) => wallet.selectedAssetFilter
export const selectedGroupAssetsByItem = ({ wallet }: State) =>
  wallet.selectedGroupAssetsByItem
export const selectedNetworkFilter = ({ wallet }: State) => wallet.selectedNetworkFilter
export const solFeeEstimates = ({ wallet }: State) => wallet.solFeeEstimates
export const userVisibleTokensInfo = ({ wallet }: State) => wallet.userVisibleTokensInfo
export const selectedAccountFilter = ({ wallet }: State) => wallet.selectedAccountFilter
export const removedFungibleTokenIds = ({ wallet }: State) =>
  wallet.removedFungibleTokenIds
export const removedNonFungibleTokenIds = ({ wallet }: State) =>
  wallet.removedNonFungibleTokenIds
export const deletedNonFungibleTokenIds = ({ wallet }: State) =>
  wallet.deletedNonFungibleTokenIds
export const isRefreshingNetworksAndTokens = ({ wallet }: State) =>
  wallet.isRefreshingNetworksAndTokens
