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
import { AccountsMenu } from './accounts-menu'

const meta: Meta<typeof AccountsMenu> = {
  component: AccountsMenu,
  args: {},
  render: (args) => {
    return (
      <WalletPanelStory>
        <AccountsMenu />
      </WalletPanelStory>
    )
  }
} satisfies Meta<typeof AccountsMenu>

export default meta
type Story = StoryObj<typeof meta>

export const _AccountsMenu: Story = {} satisfies Story
