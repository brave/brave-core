/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { setIconBasePath } from '@brave/leo/react/icon'

import { createShieldsStore } from './lib/shields_store_browser'
import { ShieldsContext } from './lib/shields_context'
import { App } from './components/app'

setIconBasePath('//resources/brave-icons')

createRoot(document.getElementById('root')!).render(
  <ShieldsContext.Provider value={createShieldsStore()}>
    <App />
  </ShieldsContext.Provider>,
)
