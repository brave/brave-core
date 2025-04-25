/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleProvider } from '../context/locale_context'
import { NewTabProvider } from '../context/new_tab_context'
import { BackgroundProvider } from '../context/background_context'
import { SearchProvider } from '../context/search_context'
import { TopSitesProvider } from '../context/top_sites_context'
import { VpnProvider } from '../context/vpn_context'
import { RewardsProvider } from '../context/rewards_context'

import { createLocale } from './storybook_locale'
import { createBackgroundAPI } from './background_impl'
import { createNewTabAPI } from './new_tab_impl'
import { createRewardsAPI } from './rewards_impl'
import { createSearchAPI } from './search_impl'
import { createTopSitesAPI } from './top_sites_impl'
import { createVpnAPI } from './vpn_impl'

import { App } from '../components/app'

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
                    {props.children}
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
