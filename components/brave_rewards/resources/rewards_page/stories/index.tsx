/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { scoped } from '../lib/scoped_css'
import { AppModelContext } from '../lib/app_model_context'
import { createModel } from './storybook_model'
import { App } from '../components/app'

export default {
  title: 'Rewards/Page'
}

const style = scoped.css`
  & {
    position: absolute;
    inset: 0;
  }
`

export function RewardsPage() {
  const model = React.useMemo(() => createModel(), [])
  return (
    <LocaleContext.Provider value={model}>
      <AppModelContext.Provider value={model}>
        <div {...style}>
          <App />
        </div>
      </AppModelContext.Provider>
    </LocaleContext.Provider>
  )
}
