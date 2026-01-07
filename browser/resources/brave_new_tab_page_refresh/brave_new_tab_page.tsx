/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { NewTabProvider } from './context/new_tab_context'
import { BackgroundProvider } from './context/background_context'
import { SearchProvider } from './context/search_context'
import { TopSitesProvider } from './context/top_sites_context'
import { VpnProvider } from './context/vpn_context'
import { RewardsProvider } from './context/rewards_context'
import { NewsProvider } from './context/news_context'

import { createNewTabState } from './state/webui_new_tab_state'
import { createBackgroundState } from './state/webui_background_state'
import { createSearchState } from './state/webui_search_state'
import { createTopSitesState } from './state/webui_top_sites_state'
import { createVpnState } from './state/webui_vpn_state'
import { createRewardsState } from './state/webui_rewards_state'

import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

function AppProvider(props: { children: React.ReactNode }) {
  return (
    <NewTabProvider
      name='newTab'
      value={createNewTabState()}
    >
      <BackgroundProvider
        name='background'
        value={createBackgroundState()}
      >
        <SearchProvider
          name='search'
          value={createSearchState()}
        >
          <TopSitesProvider
            name='topSites'
            value={createTopSitesState()}
          >
            <VpnProvider
              name='vpn'
              value={createVpnState()}
            >
              <RewardsProvider
                name='rewards'
                value={createRewardsState()}
              >
                <NewsProvider>{props.children}</NewsProvider>
              </RewardsProvider>
            </VpnProvider>
          </TopSitesProvider>
        </SearchProvider>
      </BackgroundProvider>
    </NewTabProvider>
  )
}

createRoot(document.getElementById('root')!).render(
  <AppProvider>
    <App />
  </AppProvider>,
)
