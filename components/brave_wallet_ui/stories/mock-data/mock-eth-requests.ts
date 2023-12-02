// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { mockAccount, mockNetwork } from '../../common/constants/mocks'
import { BraveWallet } from '../../constants/types'
import { mockEthMainnet } from './mock-networks'
import { mockOriginInfo } from './mock-origin-info'
import { mockEthAccount } from './mock-wallet-accounts'

export const mockSignMessageRequest = {
  originInfo: mockOriginInfo,
  id: -1,
  accountId: mockEthAccount.accountId,
  signData: {
    ethStandardSignData: {
      message: ''
    },
    ethSignTypedData: {
      domain: 'domain.com',
      domainHash: 'domainHash',
      message: 'message',
      primaryHash: 'primary hash',
      meta: {
        cowSwapOrder: {
          buyAmount: '100000',
          buyToken: 'BAT',
          deadline: new Date().toISOString(),
          receiver: mockAccount.address,
          sellAmount: '1',
          sellToken: 'ETH'
        }
      }
    },
    ethSiweData: {
      address: mockAccount.address,
      chainId: BigInt(mockEthMainnet.chainId),
      expirationTime: new Date().toISOString(),
      issuedAt: new Date().toISOString(),
      nonce: '1',
      notBefore: new Date().toISOString(),
      origin: {
        host: 'host',
        nonceIfOpaque: undefined,
        port: 8080,
        scheme: 'scheme'
      },
      requestId: '123',
      resources: [
        {
          url: 'url.com'
        }
      ],
      statement: 'statement',
      uri: {
        url: 'url.com'
      },
      version: 1
    },
    solanaSignData: undefined
  },
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID
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
