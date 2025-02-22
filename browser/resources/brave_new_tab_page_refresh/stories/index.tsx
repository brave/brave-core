/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../components/context/locale_context'
import { NewTabContext } from '../components/context/new_tab_context'
import { SearchContext } from '../components/context/search_context'
import { TopSitesContext } from '../components/context/top_sites_context'
import { VPNContext } from '../components/context/vpn_context'
import { RewardsContext } from '../components/context/rewards_context'
import { createNewTabModel } from './sb_new_tab_model'
import { createSearchModel } from './sb_search_model'
import { createTopSitesModel } from './sb_top_sites_model'
import { createVPNModel } from './sb_vpn_model'
import { createRewardsModel } from './sb_rewards_model'
import { createLocale } from './sb_locale'
import { App } from '../components/app'

export default {
  title: 'New Tab/Next'
}

export function NTPNext() {
  return (
    <LocaleContext locale={createLocale()}>
      <NewTabContext model={createNewTabModel()}>
        <SearchContext model={createSearchModel()}>
          <TopSitesContext model={createTopSitesModel()}>
            <VPNContext model={createVPNModel()}>
              <RewardsContext model={createRewardsModel()}>
                <div style={{ position: 'absolute', inset: 0 }}>
                  <App />
                </div>
              </RewardsContext>
            </VPNContext>
          </TopSitesContext>
        </SearchContext>
      </NewTabContext>
    </LocaleContext>
  )
}
