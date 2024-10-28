/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { LocaleContext } from './components/context/locale_context'
import { NewTabContext } from './components/context/new_tab_context'
import { createNewTabModel } from './webui/webui_new_tab_model'
import { SearchContext } from './components/context/search_context'
import { createSearchModel } from './webui/webui_search_model'
import { TopSitesContext } from './components/context/top_sites_context'
import { createTopSitesModel } from './webui/webui_top_sites_model'
import { RewardsContext } from './components/context/rewards_context'
import { createRewardsModel } from './webui/webui_rewards_model'
import { VPNContext } from './components/context/vpn_context'
import { createVPNModel } from './webui/webui_vpn_model'
import { createLocale } from './webui/webui_locale'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const newTabModel = createNewTabModel()
const searchModel = createSearchModel()
const topSitesModel = createTopSitesModel()
const rewardsModel = createRewardsModel()
const vpnModel = createVPNModel()

Object.assign(self, {
  [Symbol.for('ntpInternals')]: {
    newTabModel,
    searchModel,
    topSitesModel,
    rewardsModel,
    vpnModel
  }
})

createRoot(document.getElementById('root')!).render(
  <LocaleContext locale={createLocale()}>
    <NewTabContext model={newTabModel}>
      <SearchContext model={searchModel}>
        <TopSitesContext model={topSitesModel}>
          <RewardsContext model={rewardsModel}>
            <VPNContext model={vpnModel}>
              <App />
            </VPNContext>
          </RewardsContext>
        </TopSitesContext>
      </SearchContext>
    </NewTabContext>
  </LocaleContext>
)
