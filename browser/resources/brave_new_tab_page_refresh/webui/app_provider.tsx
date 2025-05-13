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

import { createLocale } from './webui_locale'
import { createBackgroundAPI } from './webui_backgrounds'
import { createNewTabAPI } from './webui_new_tab'
import { createRewardsAPI } from './webui_rewards'
import { createSearchAPI } from './webui_search'
import { createTopSitesAPI   } from './webui_top_sites'
import { createVpnAPI } from './webui_vpn'

const newTab = createNewTabAPI()
const backgrounds = createBackgroundAPI()
const rewards = createRewardsAPI()
const search = createSearchAPI()
const topSites = createTopSitesAPI()
const vpn = createVpnAPI()

// Expose APIs on the window object for console debugging.
Object.assign(window, {
  [Symbol.for('ntp')]: {
    newTab,
    backgrounds,
    rewards,
    search,
    topSites,
    vpn
  }
})

export function AppProvider(props: { children: React.ReactNode }) {
  return (
    <LocaleProvider value={createLocale()}>
      <NewTabProvider value={newTab}>
        <BackgroundProvider value={backgrounds}>
          <CurrentBackgroundProvider getRandomValue={Math.random}>
            <SearchProvider value={search}>
              <TopSitesProvider value={topSites}>
                <VpnProvider value={vpn}>
                  <RewardsProvider value={rewards}>
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
