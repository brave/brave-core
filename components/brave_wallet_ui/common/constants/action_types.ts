// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  AccountTransactions,
  BraveWallet,
  Origin,
  SlippagePresetObjectType,
  TransactionProviderError,
  WalletAccountType,
  WalletState
} from '../../constants/types'
import { RefreshedKeyringInfo, RefreshedNetworkInfo } from '../async/lib'

export type UnlockWalletPayloadType = {
  password: string
}

export type ChainChangedEventPayloadType = {
  chainId: string
  coin: BraveWallet.CoinType
}

export type IsEip1559Changed = {
  chainId: string
  isEip1559: boolean
}

export type NewUnapprovedTxAdded = {
  txInfo: BraveWallet.TransactionInfo
}

export type UnapprovedTxUpdated = {
  txInfo: BraveWallet.TransactionInfo
}

export type TransactionStatusChanged = {
  txInfo: BraveWallet.TransactionInfo
}

export type SetUserAssetVisiblePayloadType = {
  token: BraveWallet.BlockchainToken
  isVisible: boolean
}

export type SwapParamsPayloadType = {
  fromAsset: BraveWallet.BlockchainToken
  toAsset: BraveWallet.BlockchainToken
  fromAssetAmount?: string
  toAssetAmount?: string
  slippageTolerance: SlippagePresetObjectType
  accountAddress: string
  networkChainId: string
  full: boolean
}

export type UpdateUnapprovedTransactionGasFieldsType = {
  txMetaId: string
  gasLimit: string
  gasPrice?: string
  maxPriorityFeePerGas?: string
  maxFeePerGas?: string
}

export type UpdateUnapprovedTransactionSpendAllowanceType = {
  txMetaId: string
  spenderAddress: string
  allowance: string
}

export type UpdateUnapprovedTransactionNonceType = {
  txMetaId: string
  nonce: string
}

export type DefaultWalletChanged = {
  defaultWallet: BraveWallet.DefaultWallet
}

export type DefaultBaseCurrencyChanged = {
  currency: string
}

export type DefaultBaseCryptocurrencyChanged = {
  cryptocurrency: string
}

export type SitePermissionsPayloadType = {
  accounts: WalletAccountType[]
}

export type RemoveSitePermissionPayloadType = {
  origin: Origin
  account: string
}

export type AddSitePermissionPayloadType = {
  origin: Origin
  account: string
}

export type SetTransactionProviderErrorType = {
  transaction: BraveWallet.TransactionInfo
  providerError: TransactionProviderError
}

export interface SetAssetBalancesPayload {
  tokenBalances: []
  nativeAssetBalances: []
}

export interface BalancesAndPricesRefreshedPayload {
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  accounts: WalletAccountType[]
  selectedAccount: WalletAccountType
  transactionSpotPrices: BraveWallet.AssetPrice[]
}

export type WalletInfoUpdatedPayload = {
  isMetaMaskInstalled?: boolean
  connectedAccounts?: WalletAccountType[]
  fullTokenList?: BraveWallet.BlockchainToken[]
  accounts: WalletAccountType[]
  defaultAccounts: BraveWallet.AccountInfo[]
  selectedAccount: WalletAccountType
  defaultWallet?: number
} & RefreshedKeyringInfo & Partial<TransactionHistoryRefreshedPayload> & Partial<RefreshedNetworkInfo>

export interface ChainChangeCompletedPayload {
  selectedAccount: WalletAccountType
  coin: number
}

export interface SelectAccountCompletedPayload {
  coin: number
  pendingTransactions: BraveWallet.TransactionInfo[]
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.NetworkInfo
  selectedPendingTransaction?: BraveWallet.TransactionInfo
  transactions: AccountTransactions
}

export interface AccountUpdatedPayload {
  accounts: WalletAccountType[]
  selectedAccount: WalletAccountType
}

export interface WalletDataInitializedPayload {
  defaultCurrencies?: WalletState['defaultCurrencies']
  defaultNetworks?: WalletState['defaultNetworks']
  networkList?: WalletState['networkList']
  selectedNetwork?: WalletState['selectedNetwork']
  userVisibleTokensInfo?: WalletState['userVisibleTokensInfo']
  accounts?: WalletState['accounts']
  selectedAccount?: WalletState['selectedAccount']
  transactionSpotPrices?: WalletState['transactionSpotPrices']
  isFetchingPortfolioPriceHistory?: WalletState['isFetchingPortfolioPriceHistory']
  portfolioPriceHistory?: WalletState['portfolioPriceHistory']
  pendingTransactions?: WalletState['pendingTransactions']
  selectedPendingTransaction?: WalletState['selectedPendingTransaction']
  transactions?: WalletState['transactions']
}

export interface BalancesAndPricesHistoryRefreshedPayload {
  userVisibleTokensInfo?: WalletState['userVisibleTokensInfo']
  selectedAccount?: WalletState['selectedAccount']
  transactionSpotPrices?: WalletState['transactionSpotPrices']
  portfolioPriceHistory?: WalletState['portfolioPriceHistory']
  isFetchingPortfolioPriceHistory?: WalletState['isFetchingPortfolioPriceHistory']
}

export interface RefreshTokenPriceHistoryResult {
  portfolioPriceHistory: WalletState['portfolioPriceHistory']
  isFetchingPortfolioPriceHistory: WalletState['isFetchingPortfolioPriceHistory']
}

export interface TransactionHistoryRefreshedPayload {
  transactions: WalletState['transactions']
  pendingTransactions: WalletState['pendingTransactions']
  selectedPendingTransaction: WalletState['selectedPendingTransaction']
}
