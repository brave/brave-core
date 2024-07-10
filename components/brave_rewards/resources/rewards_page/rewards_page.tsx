/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { LocaleContext } from '../shared/lib/locale_context'
import { AppModelContext } from './lib/app_model_context'
import { createModel } from './lib/webui_model'
import { createLocaleForWebUI } from './lib/webui_locale'
import { TabOpenerContext } from '../shared/components/new_tab_link'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

function handleLegacyURLs() {
  const hash = location.hash.toLocaleLowerCase().replace(/^#/, '')
  if (hash === 'reset') {
    history.replaceState(null, '', '/reset')
  }
}

function whenDocumentReady() {
  return new Promise<void>((resolve) => {
    if (document.readyState === 'loading') {
      document.addEventListener('DOMContentLoaded', () => resolve())
    } else {
      resolve()
    }
  })
}

whenDocumentReady().then(() => {
  handleLegacyURLs()

  const model = createModel()
  const locale = createLocaleForWebUI()
  const root = createRoot(document.getElementById('root')!)

  root.render(
    <TabOpenerContext.Provider value={model}>
      <LocaleContext.Provider value={locale}>
        <AppModelContext.Provider value={model}>
          <App />
        </AppModelContext.Provider>
      </LocaleContext.Provider>
    </TabOpenerContext.Provider>
  )
})
