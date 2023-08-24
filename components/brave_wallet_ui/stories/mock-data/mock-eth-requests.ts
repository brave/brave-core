// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mockNetwork } from '../../common/constants/mocks'
import { BraveWallet } from '../../constants/types'
import { mockOriginInfo } from './mock-origin-info'
import { mockEthAccount } from './mock-wallet-accounts'

export const mockSignMessageRequest = {
  originInfo: mockOriginInfo,
  id: -1,
  accountId: mockEthAccount.accountId,
  message: '',
  isEip712: false,
  domainHash: '',
  domain: '',
  primaryHash: '',
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  messageBytes: undefined
}

export const mockAddChainRequest = {
  originInfo: mockOriginInfo,
  networkInfo: mockNetwork
}

export const mockGetEncryptionPublicKeyRequest = {
  requestId: '',
  originInfo: mockOriginInfo,
  accountId: mockEthAccount.accountId
}

export const mockDecryptRequest = {
  requestId: '',
  originInfo: mockOriginInfo,
  accountId: mockEthAccount.accountId,
  unsafeMessage: ''
}

export const mockSwitchChainRequest = {
  requestId: '',
  originInfo: mockOriginInfo,
  chainId: ''
}
