/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale, LocaleProvider } from '../lib/locale_context'
import { scoped } from '$web-common/scoped_css'
import { AppProvider } from '../lib/app_context'
import { createAppHandler } from './app_handler'
import { localeStrings } from './storybook_strings'
import { App } from '../components/app'

export default {
  title: 'Rewards/Page',
}

const style = scoped.css`
  & {
    position: absolute;
    inset: 0;
  }
`

const locale: Locale = {
  getString(key) {
    return localeStrings[key]
  },
}

export function RewardsPage() {
  return (
    <LocaleProvider value={locale}>
      <AppProvider createHandler={createAppHandler}>
        <div data-css-scope={style.scope}>
          <App />
        </div>
      </AppProvider>
    </LocaleProvider>
  )
}
