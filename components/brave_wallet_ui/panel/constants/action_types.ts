// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BraveWallet, SerializableOrigin, WalletAccountType } from '../../constants/types'

export type AccountPayloadType = {
  selectedAccounts: WalletAccountType[]
}

export type ConnectWithSitePayloadType = {
  addressToConnect: string,
  duration: BraveWallet.PermissionLifetimeOption
}

export type ShowConnectToSitePayload = {
  accounts: string[]
  originInfo: BraveWallet.OriginInfo
}

export type EthereumChainRequestPayload = {
  chainId: string
  approved: boolean
}

export type SignMessageProcessedPayload = {
  approved: boolean
  id: number
  signature?: BraveWallet.ByteArrayStringUnion
  error?: string
}

export type SignAllTransactionsProcessedPayload = {
  approved: boolean
  id: number
  signatures?: BraveWallet.ByteArrayStringUnion[]
  error?: string
}

export type SwitchEthereumChainProcessedPayload = {
  approved: boolean
  origin: SerializableOrigin
}

export type AddSuggestTokenProcessedPayload = {
  approved: boolean
  contractAddress: string
}

export type GetEncryptionPublicKeyProcessedPayload = {
  approved: boolean
  origin: SerializableOrigin
}

export type DecryptProcessedPayload = {
  approved: boolean
  origin: SerializableOrigin
}

export type CancelConnectHardwareWalletPayload = {
  accountAddress: string
  coinType: BraveWallet.CoinType
}
