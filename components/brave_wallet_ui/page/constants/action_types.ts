// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet } from '../../constants/types'

export type WalletCreatedPayloadType = {
  mnemonic: string
}

export type ShowRecoveryPhrasePayload =
  | {
      show: false
      password?: string
    }
  | {
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
