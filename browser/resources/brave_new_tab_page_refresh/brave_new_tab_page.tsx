/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { Locale, LocaleProvider } from './context/locale_context'
import { NewTabProvider } from './context/new_tab_context'
import { BackgroundProvider } from './context/background_context'
import { SearchProvider } from './context/search_context'
import { TopSitesProvider } from './context/top_sites_context'
import { VpnProvider } from './context/vpn_context'
import { RewardsProvider } from './context/rewards_context'
import { NewsProvider } from './context/news_context'

import { createBackgroundAPI } from './api/background_impl'
import { createNewTabAPI } from './api/new_tab_impl'
import { createRewardsAPI } from './api/rewards_impl'
import { createSearchAPI } from './api/search_impl'
import { createTopSitesAPI   } from './api/top_sites_impl'
import { createVpnAPI } from './api/vpn_impl'
import { createNewsAPI } from './api/news_impl'

import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const newTab = createNewTabAPI()
const backgrounds = createBackgroundAPI()
const rewards = createRewardsAPI()
const search = createSearchAPI()
const topSites = createTopSitesAPI()
const vpn = createVpnAPI()
const news = createNewsAPI()

// Expose APIs on the window object for console debugging.
Object.assign(window, {
  [Symbol.for('ntp')]: {
    newTab,
    backgrounds,
    rewards,
    search,
    topSites,
    vpn,
    news
  }
})

const locale: Locale = {
  getString(key) {
    return loadTimeData.getString(key)
  },
  async getPluralString(key, count) {
    return PluralStringProxyImpl.getInstance().getPluralString(key, count)
  }
}

createRoot(document.getElementById('root')!).render(
  <LocaleProvider value={locale}>
    <NewTabProvider value={newTab}>
      <BackgroundProvider value={backgrounds}>
        <SearchProvider value={search}>
          <TopSitesProvider value={topSites}>
            <VpnProvider value={vpn}>
              <RewardsProvider value={rewards}>
                <NewsProvider value={news}>
                  <App />
                </NewsProvider>
              </RewardsProvider>
            </VpnProvider>
          </TopSitesProvider>
        </SearchProvider>
      </BackgroundProvider>
    </NewTabProvider>
  </LocaleProvider>
)
