// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet, WalletState } from '../../constants/types'

// mocks
import { getNetworkId } from '../../common/slices/entities/network.entity'

export const mockWalletState: WalletState = {
  addUserAssetError: false,
  hasInitialized: true,
  isBitcoinEnabled: true,
  isBitcoinImportEnabled: true,
  isBitcoinLedgerEnabled: true,
  isZCashEnabled: true,
  isAnkrBalancesFeatureEnabled: false,
  allowedNewWalletAccountTypeNetworkIds: [
    getNetworkId({
      chainId: BraveWallet.FILECOIN_MAINNET,
    }),
    getNetworkId({
      chainId: BraveWallet.FILECOIN_TESTNET,
    }),
    getNetworkId({
      chainId: BraveWallet.BITCOIN_MAINNET,
    }),
    getNetworkId({
      chainId: BraveWallet.BITCOIN_TESTNET,
    }),
    getNetworkId({
      chainId: BraveWallet.Z_CASH_MAINNET,
    }),
    getNetworkId({
      chainId: BraveWallet.Z_CASH_TESTNET,
    }),
    getNetworkId({
      chainId: BraveWallet.SOLANA_MAINNET,
    }),
    getNetworkId({
      chainId: BraveWallet.MAINNET_CHAIN_ID,
    }),
  ],
  isWalletCreated: false,
  isWalletLocked: false,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isRefreshingNetworksAndTokens: false,
  isZCashShieldedTransactionsEnabled: false,
  isZCashIronwoodEnabled: false,
  isCardanoEnabled: true,
  isCardanoDappSupportEnabled: true,
  isPolkadotEnabled: true,
}
