// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

// types
import { BraveWallet, WalletState } from '../../constants/types'

// mocks
import { networkEntityAdapter } from '../../common/slices/entities/network.entity'

export const mockWalletState: WalletState = {
  addUserAssetError: false,
  hasInitialized: true,
  isBitcoinEnabled: true,
  isBitcoinImportEnabled: true,
  isBitcoinLedgerEnabled: true,
  isZCashEnabled: true,
  isAnkrBalancesFeatureEnabled: false,
  allowedNewWalletAccountTypeNetworkIds: [
    networkEntityAdapter.selectId({
      chainId: BraveWallet.FILECOIN_MAINNET,
      coin: BraveWallet.CoinType.FIL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.FILECOIN_TESTNET,
      coin: BraveWallet.CoinType.FIL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.BITCOIN_MAINNET,
      coin: BraveWallet.CoinType.BTC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.BITCOIN_TESTNET,
      coin: BraveWallet.CoinType.BTC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.Z_CASH_MAINNET,
      coin: BraveWallet.CoinType.ZEC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.Z_CASH_TESTNET,
      coin: BraveWallet.CoinType.ZEC
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.SOLANA_MAINNET,
      coin: BraveWallet.CoinType.SOL
    }),
    networkEntityAdapter.selectId({
      chainId: BraveWallet.MAINNET_CHAIN_ID,
      coin: BraveWallet.CoinType.ETH
    })
  ],
  isWalletCreated: false,
  isWalletLocked: false,
  passwordAttempts: 0,
  assetAutoDiscoveryCompleted: false,
  isRefreshingNetworksAndTokens: false,
  isZCashShieldedTransactionsEnabled: false
}
