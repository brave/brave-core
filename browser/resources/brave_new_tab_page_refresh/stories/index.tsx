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

import { createNewTabHandler } from './new_tab_handler'
import { createBackgroundHandler } from './background_handler'
import { createRewardsHandler } from './rewards_handler'
import { createSearchHandler } from './search_handler'
import { createTopSitesHandler } from './top_sites_handler'
import { createVpnHandler } from './vpn_handler'

import { StorybookArgs } from './storybook_args'

import { App } from '../components/app'

function StorybookApp(props: StorybookArgs) {
  return (
    <NewTabProvider createHandler={createNewTabHandler}>
      <BackgroundProvider
        createHandler={(s) => createBackgroundHandler(s, props)}
      >
        <SearchProvider createHandler={createSearchHandler}>
          <TopSitesProvider createHandler={createTopSitesHandler}>
            <VpnProvider createHandler={createVpnHandler}>
              <RewardsProvider createHandler={createRewardsHandler}>
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
