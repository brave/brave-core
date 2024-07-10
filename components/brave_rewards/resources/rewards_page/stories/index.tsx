/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import {
  LocaleContext,
  createLocaleContextForTesting
} from '../../shared/lib/locale_context'

import { AppModelContext } from '../lib/app_model_context'
import { Router, RouterContext } from '../lib/router'
import { createModel } from './storybook_model'
import { localeStrings } from '../lib/locale_strings'
import { App } from '../components/app'

export default {
  title: 'Rewards/Page'
}

export function RewardsPage() {
  const router = React.useMemo(() => {
    let currentRoute = '/'
    return new Router({
      get() { return currentRoute },
      set(route) { currentRoute = route },
      replace(route) { currentRoute = route }
    })
  }, [])

  const model = React.useMemo(() => createModel(), [])

  const locale = React.useMemo(
    () => createLocaleContextForTesting(localeStrings), [])

  return (
    <RouterContext.Provider value={router}>
      <LocaleContext.Provider value={locale}>
        <AppModelContext.Provider value={model}>
          <App />
        </AppModelContext.Provider>
      </LocaleContext.Provider>
    </RouterContext.Provider>
  )
}
