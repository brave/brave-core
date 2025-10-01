/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { AppModelContext } from '../lib/app_model_context'
import { createModel } from './storybook_model'
import { scoped } from '../../rewards_page/lib/scoped_css'
import { App } from '../components/app'

export default {
  title: 'Rewards/Internals',
}

const style = scoped.css`
  & {
    position: absolute;
    inset: 0;
  }
`

export function RewardsInternals() {
  const model = React.useMemo(() => createModel(), [])
  return (
    <AppModelContext.Provider value={model}>
      <div data-css-scope={style.scope}>
        <App />
      </div>
    </AppModelContext.Provider>
  )
}
