// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  CoinType
} from '../../constants/types'

export type CreateWalletPayloadType = {
  password: string
}

export type AddAccountPayloadType = {
  accountName: string
  keyringId: BraveWallet.KeyringId
  coin: CoinType
}

export type ImportAccountPayloadType = {
  accountName: string
  privateKey: string
  coin: CoinType
}

export type ImportFilecoinAccountPayloadType = {
  accountName: string
  privateKey: string
  network: string
}

export type ImportAccountFromJsonPayloadType = {
  accountName: string
  password: string
  json: string
}

export type RemoveAccountPayloadType = {
  accountId: BraveWallet.AccountId
  password: string
}

export type RestoreWalletPayloadType = {
  mnemonic: string
  password: string
  isLegacy: boolean
  completeWalletSetup?: boolean
}

export type WalletCreatedPayloadType = {
  mnemonic: string
}

export type ShowRecoveryPhrasePayload = {
  show: false
  password?: string
} | {
  show: true
  password: string
}

export type RecoveryWordsAvailablePayloadType = {
  mnemonic: string
}

export type UpdateSelectedAssetType = {
  asset?: BraveWallet.BlockchainToken | undefined
  timeFrame: BraveWallet.AssetPriceTimeframe
}

export type ImportFromExternalWalletPayloadType = {
  password: string
  newPassword: string
}

export type ImportWalletErrorPayloadType = {
  hasError: boolean
  errorMessage?: string
  incrementAttempts?: boolean
}

export type PinningStatusType = {
  code: BraveWallet.TokenPinStatusCode | undefined
  error: BraveWallet.PinError | undefined
}

export type NftsPinningStatusType = {
  [key: string]: PinningStatusType
}

export type UpdateNftPinningStatusType = {
  token: BraveWallet.BlockchainToken
  status?: BraveWallet.TokenPinStatus | undefined
  error?: BraveWallet.PinError | undefined
}
