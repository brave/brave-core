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
import { BraveRewardsProxyOverrides } from '$wallet/constants/testing_types'
import { WalletStatus } from '$wallet/constants/types'

type StorybookCardHubArgs = {
  isWalletCreated: boolean
  isRewardsEnabled: boolean
}

const mockRewardsInfo: BraveRewardsProxyOverrides = {
  rewardsEnabled: true,
  balance: 112.5637,
  externalWallet: {
    url: '',
    name: 'DeadBeef',
    provider: 'uphold',
    status: WalletStatus.kConnected,
  },
}

export const _CardHub = {
  render: (args: StorybookCardHubArgs) => {
    const { isWalletCreated, isRewardsEnabled } = args
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
        rewardsApiOverrides={
          isRewardsEnabled
            ? mockRewardsInfo
            : { rewardsEnabled: false, externalWallet: null }
        }
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
    isRewardsEnabled: false,
  },
  argTypes: {
    isWalletCreated: { control: { type: 'boolean' } },
    isRewardsEnabled: { control: { type: 'boolean' } },
  },
}
