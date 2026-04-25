// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Mocks
import {
  mockSignMessageRequest, //
} from '../../../stories/mock-data/mock-eth-requests'
import { mockEthAccount } from '../../../common/constants/mocks'

// components
import {
  WalletPanelStory, //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import {
  SignInWithEthereum, //
} from '../sign_in_with_ethereum/sign_in_with_ethereum'
import {
  SignInWithEthereumError, //
} from '../sign_in_with_ethereum/sign_in_with_ethereum_error'

const mockSignInWithEthereumRequest = {
  ...mockSignMessageRequest,
  accountId: mockEthAccount.accountId,
}

export const _SignInWithEthereumErrorPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <SignInWithEthereumError />
      </WalletPanelStory>
    )
  },
}

export const _SignInWithEthereumPanel = {
  render: () => {
    return (
      <WalletPanelStory>
        <SignInWithEthereum data={mockSignInWithEthereumRequest} />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Sign In With Ethereum Panels',
  parameters: {
    layout: 'centered',
  },
}
