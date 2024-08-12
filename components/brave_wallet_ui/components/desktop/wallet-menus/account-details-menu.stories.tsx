// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta, StoryObj } from '@storybook/react'

// options
import {
  AccountDetailsMenuOptions //
} from '../../../options/account-details-menu-options'

// components
import {
  WalletPanelStory //
} from '../../../stories/wrappers/wallet-panel-story-wrapper'
import { AccountDetailsMenu } from './account-details-menu'

const meta: Meta<typeof AccountDetailsMenu> = {
  component: AccountDetailsMenu,
  args: {},
  render: (args) => {
    return (
      <WalletPanelStory>
        <AccountDetailsMenu
          onClickMenuOption={(option) => {
            alert(option)
          }}
          options={AccountDetailsMenuOptions}
        />
      </WalletPanelStory>
    )
  }
} satisfies Meta<typeof AccountDetailsMenu>

export default meta
type Story = StoryObj<typeof meta>

export const _AccountDetailsMenu: Story = {} satisfies Story
