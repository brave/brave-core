/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleProvider } from '../components/context/locale_context'
import { NewTabProvider } from '../components/context/new_tab_context'
import { BackgroundProvider } from '../components/context/background_context'
import { SearchProvider } from '../components/context/search_context'
import { TopSitesProvider } from '../components/context/top_sites_context'
import { VpnProvider } from '../components/context/vpn_context'
import { RewardsProvider } from '../components/context/rewards_context'
import { NewsProvider } from '../components/context/news_context'

import { createLocale } from './webui_locale'
import { createBackgroundAPI } from './webui_backgrounds'
import { createNewTabAPI } from './webui_new_tab'
import { createRewardsAPI } from './webui_rewards'
import { createSearchAPI } from './webui_search'
import { createTopSitesAPI   } from './webui_top_sites'
import { createVpnAPI } from './webui_vpn'
import { createNewsAPI } from './webui_news'

const newTab = createNewTabAPI()
const backgrounds = createBackgroundAPI()
const rewards = createRewardsAPI()
const search = createSearchAPI()
const topSites = createTopSitesAPI()
const vpn = createVpnAPI()
const news = createNewsAPI()

Object.assign(self, {
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

export function AppProvider(props: { children: React.ReactNode }) {
  return (
    <LocaleProvider value={createLocale()}>
      <NewTabProvider value={newTab}>
        <BackgroundProvider value={backgrounds}>
          <SearchProvider value={search}>
            <TopSitesProvider value={topSites}>
              <VpnProvider value={vpn}>
                <RewardsProvider value={rewards}>
                  <NewsProvider value={news}>
                    {props.children}
                  </NewsProvider>
                </RewardsProvider>
              </VpnProvider>
            </TopSitesProvider>
          </SearchProvider>
        </BackgroundProvider>
      </NewTabProvider>
    </LocaleProvider>
  )
}
