// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { WalletPageState } from '../../constants/types'

type State = Omit<WalletPageState, 'page'>

// safe selectors (primitive return types only)
export const addUserAssetError = ({ wallet }: State) => wallet.addUserAssetError
export const hasInitialized = ({ wallet }: State) => wallet.hasInitialized
export const isBitcoinEnabled = ({ wallet }: State) => wallet.isBitcoinEnabled
export const isZCashEnabled = ({ wallet }: State) => wallet.isZCashEnabled
export const isWalletCreated = ({ wallet }: State) => wallet.isWalletCreated
export const isWalletLocked = ({ wallet }: State) => wallet.isWalletLocked
export const passwordAttempts = ({ wallet }: State) => wallet.passwordAttempts
export const assetAutoDiscoveryCompleted = ({ wallet }: State) =>
  wallet.assetAutoDiscoveryCompleted
export const isNftPinningFeatureEnabled = ({ wallet }: State) =>
  wallet.isNftPinningFeatureEnabled
export const hidePortfolioGraph = ({ wallet }: State) =>
  wallet.hidePortfolioGraph
export const hidePortfolioBalances = ({ wallet }: State) =>
  wallet.hidePortfolioBalances
export const hidePortfolioNFTsTab = ({ wallet }: State) =>
  wallet.hidePortfolioNFTsTab
export const removedNonFungibleTokens = ({ wallet }: State) =>
  wallet.removedNonFungibleTokens
export const filteredOutPortfolioNetworkKeys = ({ wallet }: State) =>
  wallet.filteredOutPortfolioNetworkKeys
export const filteredOutPortfolioAccountIds = ({ wallet }: State) =>
  wallet.filteredOutPortfolioAccountIds
export const hidePortfolioSmallBalances = ({ wallet }: State) =>
  wallet.hidePortfolioSmallBalances
export const showNetworkLogoOnNfts = ({ wallet }: State) =>
  wallet.showNetworkLogoOnNfts
export const isAnkrBalancesFeatureEnabled = ({ wallet }: State) =>
  wallet.isAnkrBalancesFeatureEnabled
export const selectedOnRampAssetId = ({ wallet }: State) =>
  wallet.selectedDepositAssetId
export const allowedNewWalletAccountTypeNetworkIds = ({ wallet }: State) =>
  wallet.allowedNewWalletAccountTypeNetworkIds

// unsafe selectors (will cause re-render if not strictly equal "===") (objects
// and lists)
export const activeOrigin = ({ wallet }: State) => wallet.activeOrigin
export const fullTokenList = ({ wallet }: State) => wallet.fullTokenList
export const selectedAssetFilter = ({ wallet }: State) =>
  wallet.selectedAssetFilter
export const selectedGroupAssetsByItem = ({ wallet }: State) =>
  wallet.selectedGroupAssetsByItem
export const selectedNetworkFilter = ({ wallet }: State) =>
  wallet.selectedNetworkFilter
export const userVisibleTokensInfo = ({ wallet }: State) =>
  wallet.userVisibleTokensInfo
export const selectedAccountFilter = ({ wallet }: State) =>
  wallet.selectedAccountFilter
export const removedFungibleTokenIds = ({ wallet }: State) =>
  wallet.removedFungibleTokenIds
export const removedNonFungibleTokenIds = ({ wallet }: State) =>
  wallet.removedNonFungibleTokenIds
export const deletedNonFungibleTokenIds = ({ wallet }: State) =>
  wallet.deletedNonFungibleTokenIds
export const isRefreshingNetworksAndTokens = ({ wallet }: State) =>
  wallet.isRefreshingNetworksAndTokens
