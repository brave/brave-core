// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../stories/wrappers/wallet-page-story-wrapper'
import SignPanel from '.'
import { mockAccounts } from '../../../stories/mock-data/mock-wallet-accounts'

import {
  BraveWallet,
  CoinType
} from '../../../constants/types'
import { mockOriginInfo } from '../../../stories/mock-data/mock-origin-info'

const signMessageData: BraveWallet.SignMessageRequest = {
  id: 0,
  address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
  message: 'To avoid digital cat burglars, sign below to authenticate with CryptoKitties.',
  originInfo: mockOriginInfo,
  coin: CoinType.ETH,
  isEip712: true,
  domainHash: '',
  domain: '',
  primaryHash: '',
  messageBytes: undefined,
  chainId: BraveWallet.MAINNET_CHAIN_ID
}

const evilUnicodeMessage = 'Sign into \u202E EVIL'

const evilUnicodeSignMessageData = {
  ...signMessageData,
  message: evilUnicodeMessage
}

export const _SignPanel = () => {
  return <WalletPageStory>
    <SignPanel
      accounts={mockAccounts}
      onCancel={() => alert('')}
      showWarning={true}
      signMessageData={[evilUnicodeSignMessageData, signMessageData]}
    />
  </WalletPageStory>
}

_SignPanel.story = {
  name: 'Sign Panel'
}

export default _SignPanel
