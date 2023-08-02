// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  CoinType,
  SerializableTransactionInfo,
  SlippagePresetObjectType,
  TransactionProviderError
} from '../../constants/types'

export type UnlockWalletPayloadType = {
  password: string
}

export type IsEip1559Changed = {
  chainId: string
  isEip1559: boolean
}

export type UnapprovedTxUpdated = {
  txInfo: SerializableTransactionInfo
}

export type TransactionStatusChanged = {
  txInfo: SerializableTransactionInfo
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
  chainId: string
  txMetaId: string
  gasLimit: string
  gasPrice?: string
  maxPriorityFeePerGas?: string
  maxFeePerGas?: string
}

export type UpdateUnapprovedTransactionSpendAllowanceType = {
  chainId: string
  txMetaId: string
  spenderAddress: string
  allowance: string
}

export type UpdateUnapprovedTransactionNonceType = {
  chainId: string
  txMetaId: string
  nonce: string
}

export type DefaultEthereumWalletChanged = {
  defaultWallet: BraveWallet.DefaultWallet
}

export type DefaultSolanaWalletChanged = {
  defaultWallet: BraveWallet.DefaultWallet
}

export type DefaultBaseCurrencyChanged = {
  currency: string
}

export type DefaultBaseCryptocurrencyChanged = {
  cryptocurrency: string
}

export type SitePermissionsPayloadType = {
  accounts: BraveWallet.AccountId[]
}

export type RemoveSitePermissionPayloadType = {
  accountId: BraveWallet.AccountId
}

export type AddSitePermissionPayloadType = {
  accountId: BraveWallet.AccountId
}

export type GetCoinMarketPayload = {
  vsAsset: string
  limit: number
}

export type GetCoinMarketsResponse = {
  success: boolean
  values: BraveWallet.CoinMarket[]
}

export type SetTransactionProviderErrorType = {
  transactionId: string
  providerError: TransactionProviderError
}

export interface RetryTransactionPayload {
  chainId: string
  transactionId: string
  coinType: CoinType
  fromAddress: string
}

export type SpeedupTransactionPayload = RetryTransactionPayload
export type CancelTransactionPayload = RetryTransactionPayload

export type UpdateUsetAssetType = {
  existing: BraveWallet.BlockchainToken
  updated: BraveWallet.BlockchainToken
}
