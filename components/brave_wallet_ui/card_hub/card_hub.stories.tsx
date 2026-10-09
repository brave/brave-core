// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import { CardHub } from './card_hub'
import {
  WalletPanelStory, //
} from '$wallet/stories/wrappers/wallet-panel-story-wrapper'

// Mocks
import {
  mockAccount,
  mockNativeBalanceRegistry,
  mockTokenBalanceRegistry,
} from '$wallet/common/constants/mocks'

type StorybookCardHubArgs = {
  isWalletCreated: boolean
}

export const _CardHub = {
  render: (args: StorybookCardHubArgs) => {
    const { isWalletCreated } = args
    return (
      <WalletPanelStory
        walletStateOverride={{
          isWalletCreated,
        }}
        walletApiDataOverrides={{
          isWalletCreated,
          selectedAccountId: mockAccount.accountId,
          nativeBalanceRegistry: mockNativeBalanceRegistry,
          tokenBalanceRegistry: mockTokenBalanceRegistry,
        }}
      >
        <CardHub />
      </WalletPanelStory>
    )
  },
}

export default {
  title: 'Wallet/Panel/Panels/Card Hub',
  component: CardHub,
  parameters: {
    layout: 'centered',
  },
  args: {
    isWalletCreated: false,
  },
  argTypes: {
    isWalletCreated: { control: { type: 'boolean' } },
  },
}
