/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import type { Meta, StoryObj } from '@storybook/react'

import '../../../../components/brave_new_tab_ui/stories/default/data/mockBraveNewsController'

import { NewTabProvider } from '../context/new_tab_context'
import { BackgroundProvider } from '../context/background_context'
import { SearchProvider } from '../context/search_context'
import { TopSitesProvider } from '../context/top_sites_context'
import { VpnProvider } from '../context/vpn_context'
import { RewardsProvider } from '../context/rewards_context'

import { createNewTabState } from './mock_new_tab_state'
import { createBackgroundState } from './mock_background_state'
import { createRewardsState } from './mock_rewards_state'
import { createSearchState } from './mock_search_state'
import { createTopSitesState } from './mock_top_sites_state'
import { createVpnState } from './mock_vpn_state'

import { StorybookArgs } from './storybook_args'

import { App } from '../components/app'

function StorybookApp(props: StorybookArgs) {
  return (
    <NewTabProvider value={createNewTabState()}>
      <BackgroundProvider value={createBackgroundState(props)}>
        <SearchProvider value={createSearchState()}>
          <TopSitesProvider value={createTopSitesState()}>
            <VpnProvider value={createVpnState()}>
              <RewardsProvider value={createRewardsState()}>
                <div style={{ position: 'absolute', inset: 0 }}>
                  <App />
                </div>
              </RewardsProvider>
            </VpnProvider>
          </TopSitesProvider>
        </SearchProvider>
      </BackgroundProvider>
    </NewTabProvider>
  )
}

export default {
  title: 'New Tab Page/App',
  component: StorybookApp,
} satisfies Meta<typeof StorybookApp>

export const NewTabPage: StoryObj<typeof StorybookApp> = {
  argTypes: {
    sponsoredBackgroundType: {
      control: 'select',
      options: ['none', 'image', 'rich'],
    },
  },
}
