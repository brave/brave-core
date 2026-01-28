/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { AppContext } from './lib/app_context'
import { createAppStore } from './lib/browser_app_store'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const root = createRoot(document.getElementById('root')!)

root.render(
  <AppContext.Provider value={createAppStore()}>
    <App />
  </AppContext.Provider>,
)
