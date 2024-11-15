/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { LocaleContext } from './components/locale_context'
import { NewTabContext } from './components/new_tab_context'
import { createNewTabModel } from './webui/webui_new_tab_model'
import { SearchContext } from './components/search_context'
import { createSearchModel } from './webui/webui_search_model'
import { createLocale } from './webui/webui_locale'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const newTabModel = createNewTabModel()
const searchModel = createSearchModel()

Object.assign(self, {
  [Symbol.for('ntpInternals')]: {
    newTabModel,
    searchModel
  }
})

createRoot(document.getElementById('root')!).render(
  <LocaleContext locale={createLocale()}>
    <NewTabContext model={newTabModel}>
      <SearchContext model={searchModel}>
        <App />
      </SearchContext>
    </NewTabContext>
  </LocaleContext>
)
