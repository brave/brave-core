// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../stories/wrappers/wallet-page-story-wrapper'
import { SignPanel } from './index'

import { BraveWallet, Url } from '../../../constants/types'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'
import { mockEthAccount } from '../../../stories/mock-data/mock-wallet-accounts'
import { PanelWrapper } from '../../../panel/style'
import { LongWrapper } from '../../../stories/style'
import WalletPanelStory from '../../../stories/wrappers/wallet-panel-story-wrapper'

let mockURLPath = new Url()
let mockIPFSPath = new Url()
const urlPath = 'https://www.test.com'
const ipftPath =
  'ipfs://bafybeiemxf5abjwjbikoz4 mc3a3dla6ual3jsgpdr4cjr3oz3evfyavwq/'
mockURLPath.url = urlPath
mockIPFSPath.url = ipftPath

const signMessageData: BraveWallet.SignMessageRequest = {
  id: 0,
  accountId: mockEthAccount.accountId,
  originInfo: mockOriginInfo,
  coin: BraveWallet.CoinType.ETH,
  chainId: BraveWallet.MAINNET_CHAIN_ID,
  signData: {
    ethSignTypedData: undefined,
    solanaSignData: undefined,
    ethSiweData: {
      address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
      expirationTime: '02/16/2024',
      issuedAt: '02/01/2024',
      nonce: '5f654',
      notBefore: '',
      requestId: '22',
      resources: [mockIPFSPath, mockURLPath],
      statement: 'I accept the Brave Terms of Service: http://address.com/tos',
      uri: {
        url: mockOriginInfo.originSpec
      },
      version: 1,
      chainId: BigInt(123),
      origin: {
        host: '',
        nonceIfOpaque: undefined,
        port: 0,
        scheme: ''
      }
    },
    ethStandardSignData: undefined
  }
}

const evilUnicodeMessage = 'Sign into \u202E EVIL'

const evilUnicodeSignMessageData = {
  ...signMessageData,
  message: evilUnicodeMessage
}

export const _SignPanel = {
  render: () => {
    return (
      <WalletPageStory>
        <PanelWrapper isLonger>
          <LongWrapper>
            <SignPanel
              showWarning={true}
              signMessageData={[evilUnicodeSignMessageData, signMessageData]}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPageStory>
    )
  }
}

export const _SignData = {
  render: () => {
    const signMessageDataPayload: BraveWallet.SignMessageRequest[] = [
      {
        id: 0,
        accountId: {
          coin: BraveWallet.CoinType.ETH,
          keyringId: BraveWallet.KeyringId.kDefault,
          kind: BraveWallet.AccountKind.kDerived,
          address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
          accountIndex: 0,
          uniqueKey: '0x3f29A1da97149722eB09c526E4eAd698895b426_id'
        },
        originInfo: mockOriginInfo,
        coin: BraveWallet.CoinType.ETH,
        chainId: BraveWallet.MAINNET_CHAIN_ID,
        signData: {
          ethStandardSignData: undefined,
          ethSignTypedData: {
            addressParam: '',
            messageJson: 'Sign below to authenticate with CryptoKitties.',
            domainJson: '',
            typesJson: '',
            primaryType: '',
            chainId: '',
            domainHash: [],
            primaryHash: [],
            meta: undefined
          },
          ethSiweData: undefined,
          solanaSignData: undefined
        }
      }
    ]

    return (
      <WalletPanelStory>
        <PanelWrapper isLonger={true}>
          <LongWrapper>
            <SignPanel
              signMessageData={signMessageDataPayload}
              showWarning={true}
            />
          </LongWrapper>
        </PanelWrapper>
      </WalletPanelStory>
    )
  }
}

export default { component: SignPanel }
