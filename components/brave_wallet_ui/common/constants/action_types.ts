// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  Origin,
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

export type SelectedAccountChangedPayloadType = {
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
  accounts: WalletAccountType[]
}

export type RemoveSitePermissionPayloadType = {
  coin: BraveWallet.CoinType
  origin: Origin
  account: string
}

export type AddSitePermissionPayloadType = {
  coin: BraveWallet.CoinType
  origin: Origin
  account: string
}

export type SetTransactionProviderErrorType = {
  transaction: BraveWallet.TransactionInfo
  providerError: TransactionProviderError
}
