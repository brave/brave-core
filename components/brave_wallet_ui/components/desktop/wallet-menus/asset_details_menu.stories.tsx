// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

// Components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { AssetDetailsMenu } from './asset-details-menu'
import { mockBasicAttentionToken } from '../../../stories/mock-data/mock-asset-options'

const meta: Meta<typeof AssetDetailsMenu> = {
  component: AssetDetailsMenu,
  args: {},
  render: (args) => {
    return (
      <WalletPanelStory>
        <AssetDetailsMenu
          assetSymbol={mockBasicAttentionToken.symbol}
          onClickHideToken={() => {
            alert('HideToken')
          }}
          onClickTokenDetails={() => {
            alert('TokenDetails')
          }}
          onClickViewOnExplorer={() => {
            alert('ViewOnExplorer')
          }}
          onClickEditToken={() => {
            alert('EditToken')
          }}
        />
      </WalletPanelStory>
    )
  }
} satisfies Meta<typeof AssetDetailsMenu>

export default meta
type Story = StoryObj<typeof meta>

export const _AssetDetailsMenu: Story = {} satisfies Story
