/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { LocaleContext } from './components/context/locale_context'
import { AppModelContext } from './components/context/app_model_context'
import { createAppModel } from './webui/webui_app_model'
import { createLocale } from './webui/webui_locale'
import { App } from './components/app'

setIconBasePath('chrome://resources/brave-icons')

const appModel = createAppModel()

Object.assign(self, {
  [Symbol.for('ntpAppModel')]: appModel
})

createRoot(document.getElementById('root')!).render(
  <LocaleContext locale={createLocale()}>
    <AppModelContext model={appModel}>
      <App />
    </AppModelContext>
  </LocaleContext>
)
