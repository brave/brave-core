/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import type { Meta, StoryObj } from '@storybook/react'

import '../../../../components/brave_new_tab_ui/stories/default/data/mockBraveNewsController'
import { setupBraveNewsStorybookMock } from './mock_brave_news_setup'

import { NewTabContext } from '../context/new_tab_context'
import { BackgroundContext } from '../context/background_context'
import { SearchContext } from '../context/search_context'
import { TopSitesContext } from '../context/top_sites_context'
import { VpnContext } from '../context/vpn_context'
import { RewardsContext } from '../context/rewards_context'
import { NewsProvider } from '../context/news_context'

import { createNewTabStore } from './mock_new_tab_store'
import { createBackgroundStore } from './mock_background_store'
import { createRewardsStore } from './mock_rewards_store'
import { createSearchStore } from './mock_search_store'
import { createTopSitesStore } from './mock_top_sites_store'
import { createVpnStore } from './mock_vpn_store'
import {
  applyStorybookArgs,
  defaultStorybookArgs,
  settingsUrlForStory,
  type StorybookArgs,
} from './storybook_args'

import { App } from '../components/app'

setupBraveNewsStorybookMock()

function StorybookApp(props: StorybookArgs) {
  const stores = React.useMemo(() => {
    return {
      newTab: createNewTabStore(),
      background: createBackgroundStore(),
      search: createSearchStore(),
      topSites: createTopSitesStore(),
      vpn: createVpnStore(),
      rewards: createRewardsStore(),
    }
  }, [])

  React.useEffect(() => {
    applyStorybookArgs(stores, props)
  }, [props, stores])

  const settingsUrl = settingsUrlForStory(props.openSettings)
  if (`${location.pathname}${location.search}` !== settingsUrl) {
    history.replaceState(null, '', settingsUrl)
  }

  return (
    <NewTabContext.Provider value={stores.newTab}>
      <BackgroundContext.Provider value={stores.background}>
        <SearchContext.Provider value={stores.search}>
          <TopSitesContext.Provider value={stores.topSites}>
            <VpnContext.Provider value={stores.vpn}>
              <RewardsContext.Provider value={stores.rewards}>
                <NewsProvider>
                  <div style={{ position: 'absolute', inset: 0 }}>
                    <App key={props.openSettings} />
                  </div>
                </NewsProvider>
              </RewardsContext.Provider>
            </VpnContext.Provider>
          </TopSitesContext.Provider>
        </SearchContext.Provider>
      </BackgroundContext.Provider>
    </NewTabContext.Provider>
  )
}

const storyArgTypes = {
  viewportWidth: {
    control: { type: 'range', min: 900, max: 1600, step: 25 },
  },
  backgroundType: {
    control: 'select',
    options: ['gradient', 'solid', 'brave', 'custom', 'disabled'],
  },
  sponsoredBackgroundType: {
    control: 'select',
    options: ['none', 'image', 'rich'],
  },
  centerNttCtaButton: { control: 'boolean' },
  showClock: { control: 'boolean' },
  clockFormat: {
    control: 'select',
    options: ['auto', '12', '24'],
  },
  showTopSites: { control: 'boolean' },
  showShieldsStats: { control: 'boolean' },
  showTalkWidget: { control: 'boolean' },
  showVpnWidget: { control: 'boolean' },
  showRewardsWidget: { control: 'boolean' },
  aiChatInputEnabled: { control: 'boolean' },
  showSearchBox: { control: 'boolean' },
  newsFeatureEnabled: { control: 'boolean' },
  newsShowOnNTP: { control: 'boolean' },
  newsOptedIn: { control: 'boolean' },
  vpnPurchased: { control: 'boolean' },
  vpnConnectionState: {
    control: 'select',
    options: ['connected', 'connecting', 'disconnected'],
  },
  rewardsEnabled: { control: 'boolean' },
  openSettings: {
    control: 'select',
    options: ['none', 'background', 'search', 'top-sites', 'clock', 'widgets', 'news'],
  },
} satisfies Meta<typeof StorybookApp>['argTypes']

export default {
  title: 'New Tab Page/App',
  component: StorybookApp,
  parameters: {
    layout: 'fullscreen',
  },
  decorators: [
    (Story, context) => (
      <div
        style={{
          width: context.args.viewportWidth,
          height: '100vh',
          margin: '0 auto',
          position: 'relative',
          overflow: 'auto',
        }}
      >
        <Story />
      </div>
    ),
  ],
  argTypes: storyArgTypes,
  args: defaultStorybookArgs,
} satisfies Meta<typeof StorybookApp>

export const Desktop: StoryObj<typeof StorybookApp> = {}
