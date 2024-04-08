// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  NativeAssetBalanceRegistry,
  TokenBalanceRegistry
} from '../common/constants/mocks'
import { BraveWallet, RewardsExternalWallet } from './types'

export interface WalletApiDataOverrides {
  selectedCoin?: BraveWallet.CoinType
  selectedAccountId?: BraveWallet.AccountId
  chainIdsForCoins?: Record<BraveWallet.CoinType, string>
  networks?: BraveWallet.NetworkInfo[]
  defaultBaseCurrency?: string
  transactionInfos?: BraveWallet.TransactionInfo[]
  blockchainTokens?: BraveWallet.BlockchainToken[]
  userAssets?: BraveWallet.BlockchainToken[]
  accountInfos?: BraveWallet.AccountInfo[]
  nativeBalanceRegistry?: NativeAssetBalanceRegistry
  tokenBalanceRegistry?: TokenBalanceRegistry
  simulationOptInStatus?: BraveWallet.BlowfishOptInStatus
  evmSimulationResponse?: BraveWallet.EVMSimulationResponse | null
  svmSimulationResponse?: BraveWallet.SolanaSimulationResponse | null
  signTransactionRequests?: BraveWallet.SignTransactionRequest[]
  signAllTransactionsRequests?: BraveWallet.SignAllTransactionsRequest[]
}

export type BraveRewardsProxyOverrides = Partial<{
  rewardsEnabled: boolean
  balance: number
  externalWallet: RewardsExternalWallet | null
}>
