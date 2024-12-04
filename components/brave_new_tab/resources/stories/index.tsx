/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../components/locale_context'
import { NewTabContext } from '../components/new_tab_context'
import { SearchContext } from '../components/search_context'
import { TopSitesContext } from '../components/top_sites_context'
import { createNewTabModel } from './sb_new_tab_model'
import { createSearchModel } from './sb_search_model'
import { createTopSitesModel } from './sb_top_sites_model'
import { createLocale } from './sb_locale'
import { App } from '../components/app'

export default {
  title: 'New Tab/Next'
}

export function NTPNext() {
  const newTabModel = React.useMemo(() => createNewTabModel(), [])
  return (
    <LocaleContext locale={createLocale()}>
      <NewTabContext model={newTabModel}>
        <SearchContext model={createSearchModel()}>
          <TopSitesContext model={createTopSitesModel()}>
            <div style={{ position: 'absolute', inset: 0 }}>
              <App />
            </div>
          </TopSitesContext>
        </SearchContext>
      </NewTabContext>
    </LocaleContext>
  )
}
