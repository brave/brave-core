// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { WalletAccountType, EthereumChain, Url } from '../../constants/types'

export type AccountPayloadType = {
  selectedAccounts: WalletAccountType[]
  siteToConnectTo: string
}

export type ShowConnectToSitePayload = {
  tabId: number
  accounts: string[]
  origin: string
}

export type EthereumChainRequestPayload = {
  chainId: string
  approved: boolean
}

export type EthereumChainPayload = {
  chain: EthereumChain
}

export type SignMessagePayload = {
  id: number
  address: string
  message: string
}

export type SignMessageProcessedPayload = {
  approved: boolean
  id: number
}

export type SignMessageHardwareProcessedPayload = {
  success: Boolean
  id: number
  signature?: string
  error?: string
}

export type SwitchEthereumChainProcessedPayload = {
  approved: boolean
  origin: Url
}
