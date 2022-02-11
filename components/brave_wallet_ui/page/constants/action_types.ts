// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  BraveWallet,
  GetPriceHistoryReturnObjectInfo
} from '../../constants/types'

export type CreateWalletPayloadType = {
  password: string
}

export type AddAccountPayloadType = {
  accountName: string
  coin: BraveWallet.CoinType
}

export type ImportAccountPayloadType = {
  accountName: string
  privateKey: string
  coin: BraveWallet.CoinType
}

export type ImportFilecoinAccountPayloadType = {
  accountName: string
  privateKey: string
  network: string
  protocol: BraveWallet.FilecoinAddressProtocol
}

export type ImportAccountFromJsonPayloadType = {
  accountName: string
  password: string
  json: string
}

export type RemoveImportedAccountPayloadType = {
  address: string
}

export type RemoveHardwareAccountPayloadType = {
  address: string
}

export type RestoreWalletPayloadType = {
  mnemonic: string
  password: string
  isLegacy: boolean
}

export type WalletCreatedPayloadType = {
  mnemonic: string
}

export type ViewPrivateKeyPayloadType = {
  isDefault: boolean
  address: string
}

export type RecoveryWordsAvailablePayloadType = {
  mnemonic: string
}

export type PrivateKeyAvailablePayloadType = {
  privateKey: string
}

export type UpdateSelectedAssetType = {
  asset: BraveWallet.BlockchainToken
  timeFrame: BraveWallet.AssetPriceTimeframe
}

export type SelectAssetPayloadType = {
  priceHistory: GetPriceHistoryReturnObjectInfo | undefined
  defaultFiatPrice?: BraveWallet.AssetPrice
  defaultCryptoPrice?: BraveWallet.AssetPrice
  timeFrame: BraveWallet.AssetPriceTimeframe
}

export type ImportFromExternalWalletPayloadType = {
  password: string
  newPassword: string
}

export type ImportWalletErrorPayloadType = {
  hasError: boolean
  errorMessage?: string
}
