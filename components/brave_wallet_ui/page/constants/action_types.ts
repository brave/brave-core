// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import {
  GetPriceHistoryReturnObjectInfo,
  TokenInfo,
  AssetPriceInfo,
  AssetPriceTimeframe
} from '../../constants/types'

export type CreateWalletPayloadType = {
  password: string
}

export type AddAccountPayloadType = {
  accountName: string
}

export type ImportAccountPayloadType = {
  accountName: string,
  privateKey: string
}

export type ImportAccountFromJsonPayloadType = {
  accountName: string,
  password: string,
  json: string
}

export type RemoveImportedAccountPayloadType = {
  address: string
}

export type RestoreWalletPayloadType = {
  mnemonic: string,
  password: string
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
  asset: TokenInfo,
  timeFrame: AssetPriceTimeframe
}

export type SelectAssetPayloadType = {
  priceHistory: GetPriceHistoryReturnObjectInfo | undefined,
  usdPriceInfo: AssetPriceInfo | undefined,
  btcPriceInfo: AssetPriceInfo | undefined,
  timeFrame: AssetPriceTimeframe
}
