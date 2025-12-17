/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppProvider } from '../components/app_context'
import { LocaleProvider } from '../components/locale_context'
import { createAppHandler } from './app_handler'
import { localeStrings } from './storybook_strings'
import { scoped } from '$web-common/scoped_css'
import { App } from '../components/app'
import { StringKey } from '../lib/locale_strings'

export default {
  title: 'Rewards/Internals',
}

const style = scoped.css`
  & {
    position: absolute;
    inset: 0;
  }
`

const locale = {
  getString(key: StringKey) {
    return localeStrings[key]
  },
}

export function RewardsInternals() {
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
