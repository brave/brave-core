/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { AppContext } from './lib/app_context'
import { createAppStore } from './webui/browser_app_store'
import { TabOpenerContext } from '../shared/components/new_tab_link'
import { ShowHandler } from './components/common/show_handler'
import { App } from './components/app'
import * as routes from './lib/app_routes'

setIconBasePath('chrome://resources/brave-icons')

function handleLegacyURLs() {
  switch (location.hash.toLocaleLowerCase().replace(/^#/, '')) {
    case 'reset':
      history.replaceState(null, '', routes.reset)
      return
    case 'verify':
      history.replaceState(null, '', routes.connectAccount)
      return
  }

  if (location.pathname === '/reconnect') {
    history.replaceState(null, '', routes.connectAccount)
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

  const store = createAppStore()
  const { actions } = store.getState()
  const root = createRoot(document.getElementById('root')!)

  root.render(
    <TabOpenerContext.Provider value={actions}>
      <AppContext.Provider value={store}>
        <ShowHandler>
          <App />
        </ShowHandler>
      </AppContext.Provider>
    </TabOpenerContext.Provider>,
  )
})
