// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import {
  AppObjectType,
  AccountInfo,
  TransactionStatus
} from '../../constants/types'

export type InitializedPayloadType = {
  isWalletCreated: boolean
  isWalletLocked: boolean
  favoriteApps: AppObjectType[]
  isWalletBackedUp: boolean
  visibleTokens: string[]
  accountInfos: AccountInfo[]
}

export type UnlockWalletPayloadType = {
  password: string
}

export type SetInitialVisibleTokensPayloadType = {
  visibleAssets: string[]
}

export type ChainChangedEventPayloadType = {
  chainId: string
}

export type NewUnapprovedTxAdded = {
  txMetaId: string
}

export type TransactionStatusChanged = {
  txMetaId: string
  txStatus: TransactionStatus
}
