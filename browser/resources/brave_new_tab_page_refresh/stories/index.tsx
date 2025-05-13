/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleProvider } from '../context/locale'
import { NewTabProvider } from '../context/new_tab'
import { BackgroundProvider } from '../context/backgrounds'
import { CurrentBackgroundProvider } from '../context/current_background'
import { SearchProvider } from '../context/search'
import { TopSitesProvider } from '../context/top_sites'
import { VpnProvider } from '../context/vpn'
import { RewardsProvider } from '../context/rewards'

import { createLocale } from './sb_locale'
import { createBackgroundAPI } from './sb_backgrounds'
import { createNewTabAPI } from './sb_new_tab'
import { createRewardsAPI } from './sb_rewards'
import { createSearchAPI } from './sb_search'
import { createTopSitesAPI } from './sb_top_sites'
import { createVpnAPI } from './sb_vpn'

import { App } from '../components/app'

export default {
  title: 'New Tab/Refresh'
}

function AppProvider(props: { children: React.ReactNode }) {
  return (
    <LocaleProvider value={createLocale()}>
      <NewTabProvider value={createNewTabAPI()}>
        <BackgroundProvider value={createBackgroundAPI()}>
          <CurrentBackgroundProvider getRandomValue={Math.random}>
            <SearchProvider value={createSearchAPI()}>
              <TopSitesProvider value={createTopSitesAPI()}>
                <VpnProvider value={createVpnAPI()}>
                  <RewardsProvider value={createRewardsAPI()}>
                    {props.children}
                  </RewardsProvider>
                </VpnProvider>
              </TopSitesProvider>
            </SearchProvider>
          </CurrentBackgroundProvider>
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
