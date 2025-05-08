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

import { createLocale } from './sb_locale'
import { createBackgroundAPI } from './sb_backgrounds'
import { createNewTabAPI } from './sb_new_tab'
import { createRewardsAPI } from './sb_rewards'
import { createSearchAPI } from './sb_search'
import { createTopSitesAPI } from './sb_top_sites'
import { createVpnAPI } from './sb_vpn'
import { createNewsAPI } from './sb_news'

import { App, NewsApp } from '../components/app'

export default {
  title: 'New Tab/Refresh'
}

function AppProvider(props: { children: React.ReactNode }) {
  return (
    <LocaleProvider value={createLocale()}>
      <NewTabProvider value={createNewTabAPI()}>
        <BackgroundProvider value={createBackgroundAPI()}>
          <SearchProvider value={createSearchAPI()}>
            <TopSitesProvider value={createTopSitesAPI()}>
              <VpnProvider value={createVpnAPI()}>
                <RewardsProvider value={createRewardsAPI()}>
                  <NewsProvider value={createNewsAPI()}>
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

export function NTPRefresh() {
  return (
    <AppProvider>
      <div style={{ position: 'absolute', inset: 0 }}>
        <App />
      </div>
    </AppProvider>
  )
}

export function NewsOnly() {
  return (
    <LocaleProvider value={createLocale()}>
      <NewsProvider value={createNewsAPI()}>
        <NewsApp />
      </NewsProvider>
    </LocaleProvider>
  )
}
