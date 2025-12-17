/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { loadTimeData } from 'chrome://resources/js/load_time_data.js'

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { AppProvider } from './components/app_context'
import { LocaleProvider } from './components/locale_context'
import { Locale } from './lib/locale_strings'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const root = createRoot(document.getElementById('root')!)

const locale: Locale = {
  getString: (key) => loadTimeData.getString(key),
}

root.render(
  <AppProvider>
    <LocaleProvider value={locale}>
      <App />
    </LocaleProvider>
  </AppProvider>,
)
