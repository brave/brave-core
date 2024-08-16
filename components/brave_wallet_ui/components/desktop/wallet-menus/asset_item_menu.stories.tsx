// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { AssetItemMenu } from './asset-item-menu'
import { mockBasicAttentionToken } from '../../../stories/mock-data/mock-asset-options'
import { mockAccount } from '../../../common/constants/mocks'

const meta: Meta<typeof AssetItemMenu> = {
  component: AssetItemMenu,
  args: {},
  render: (args) => {
    return (
      <WalletPanelStory>
        <AssetItemMenu
          asset={mockBasicAttentionToken}
          assetBalance='1000'
          account={mockAccount}
          onClickEditToken={() => alert('edit')}
        />
      </WalletPanelStory>
    )
  }
} satisfies Meta<typeof AssetItemMenu>

export default meta
type Story = StoryObj<typeof meta>

export const _AssetItemMenu: Story = {} satisfies Story
