/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { scoped } from '$web-common/scoped_css'
import { AppContext } from '../lib/app_context'
import { createAppStore } from './storybook_app_store'
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

export function RewardsPage() {
  const store = createAppStore()
  return (
    <AppContext.Provider value={store}>
      <div data-css-scope={style.scope}>
        <App />
      </div>
    </AppContext.Provider>
  )
}
