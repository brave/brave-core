// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  SlippagePresetObjectType,
  TransactionProviderError,
  WalletAccountType
} from '../../constants/types'

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

export type AddUserAssetPayloadType = {
  token: BraveWallet.BlockchainToken
  chainId: string
}

export type RemoveUserAssetPayloadType = {
  token: BraveWallet.BlockchainToken
  chainId: string
}

export type SetUserAssetVisiblePayloadType = {
  token: BraveWallet.BlockchainToken
  chainId: string
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

export type ActiveOriginChanged = {
  origin: string
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
  accounts: Array<WalletAccountType | undefined>
}

export type RemoveSitePermissionPayloadType = {
  origin: string
  account: string
}

export type AddSitePermissionPayloadType = {
  origin: string
  account: string
}

export type SetTransactionProviderErrorType = {
  transaction: BraveWallet.TransactionInfo
  providerError: TransactionProviderError
}
