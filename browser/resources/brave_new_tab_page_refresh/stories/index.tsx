/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Meta } from '@storybook/react'

import './storybook_locale'

import '../../../../components/brave_new_tab_ui/stories/default/data/mockBraveNewsController'

import { NewTabProvider } from '../context/new_tab_context'
import { BackgroundProvider } from '../context/background_context'
import { SearchProvider } from '../context/search_context'
import { TopSitesProvider } from '../context/top_sites_context'
import { VpnProvider } from '../context/vpn_context'
import { RewardsProvider } from '../context/rewards_context'

import { createNewTabHandler } from './new_tab_handler'
import { createBackgroundHandler } from './background_handler'
import { createRewardsHandler } from './rewards_handler'
import { createSearchHandler } from './search_handler'
import { createTopSitesHandler } from './top_sites_handler'
import { createVpnHandler } from './vpn_handler'

import { App } from '../components/app'

interface StorybookArgs {
  aiChatInputEnabled: boolean
}

function StorybookAppProvider(
  props: { args: StorybookArgs, children: React.ReactNode }
) {
  return (
    <NewTabProvider
      createHandler={createNewTabHandler}
      stateOverrides={{ aiChatInputEnabled: props.args.aiChatInputEnabled }}
    >
      <BackgroundProvider createHandler={createBackgroundHandler}>
        <SearchProvider createHandler={createSearchHandler}>
          <TopSitesProvider createHandler={createTopSitesHandler}>
            <VpnProvider createHandler={createVpnHandler}>
              <RewardsProvider createHandler={createRewardsHandler}>
                {props.children}
              </RewardsProvider>
            </VpnProvider>
          </TopSitesProvider>
        </SearchProvider>
      </BackgroundProvider>
    </NewTabProvider>
  )
}

const meta: Meta<StorybookArgs> = {
  title: 'New Tab/Refresh',
  argTypes: {
    aiChatInputEnabled: {
      control: 'boolean',
      name: 'AI Chat input enabled'
    }
  },
  render: (args) => {
    return (
      <StorybookAppProvider args={args}>
        <div style={{ position: 'absolute', inset: 0 }}>
          <App />
        </div>
      </StorybookAppProvider>
    )
  }
}

export default meta

export const NTPRefresh = {}
