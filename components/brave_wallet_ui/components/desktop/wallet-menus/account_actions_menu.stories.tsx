// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

// options
import {
  AccountButtonOptions //
} from '../../../options/account-list-button-options'

// components
import { AccountActionsMenu } from './account-actions-menu'
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'

const meta: Meta<typeof AccountActionsMenu> = {
  component: AccountActionsMenu,
  args: {},
  render: (args) => {
    return (
      <WalletPanelStory>
        <AccountActionsMenu
          onClick={() => alert('clicked')}
          options={AccountButtonOptions}
        />
      </WalletPanelStory>
    )
  }
} satisfies Meta<typeof AccountActionsMenu>

export default meta
type Story = StoryObj<typeof meta>

export const _AccountActionsMenu: Story = {} satisfies Story
