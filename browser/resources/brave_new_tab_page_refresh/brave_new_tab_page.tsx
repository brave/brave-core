/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { NewTabContext } from './context/new_tab_context'
import { BackgroundContext } from './context/background_context'
import { SearchContext } from './context/search_context'
import { TopSitesContext } from './context/top_sites_context'
import { VpnContext } from './context/vpn_context'
import { RewardsContext } from './context/rewards_context'
import { NewsProvider } from './context/news_context'

import { createNewTabStore } from './state/browser_new_tab_store'
import { createBackgroundStore } from './state/browser_background_store'
import { createSearchStore } from './state/browser_search_store'
import { createTopSitesStore } from './state/browser_top_sites_store'
import { createVpnStore } from './state/browser_vpn_store'
import { createRewardsStore } from './state/browser_rewards_store'

import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

function AppProvider(props: { children: React.ReactNode }) {
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
    Reflect.set(self, '_ntp', stores)
  }, [])

  return (
    <NewTabContext.Provider value={stores.newTab}>
      <BackgroundContext.Provider value={stores.background}>
        <SearchContext.Provider value={stores.search}>
          <TopSitesContext.Provider value={stores.topSites}>
            <VpnContext.Provider value={stores.vpn}>
              <RewardsContext.Provider value={stores.rewards}>
                <NewsProvider>{props.children}</NewsProvider>
              </RewardsContext.Provider>
            </VpnContext.Provider>
          </TopSitesContext.Provider>
        </SearchContext.Provider>
      </BackgroundContext.Provider>
    </NewTabContext.Provider>
  )
}

createRoot(document.getElementById('root')!).render(
  <AppProvider>
    <App />
  </AppProvider>,
)
