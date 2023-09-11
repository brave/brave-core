/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { initLocale } from 'brave-ui'
import { setIconBasePath } from '@brave/leo/react/icon'

import '$web-components/app.global.scss'
import '@brave/leo/tokens/css/variables.css'

import { loadTimeData } from '$web-common/loadTimeData'
import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'
import Main from './components/main'
import DataContextProvider from './state/data-context-provider'

setIconBasePath('chrome-untrusted://resources/brave-icons')

function App () {
  return (
    <DataContextProvider>
      <BraveCoreThemeProvider>
        <Main />
      </BraveCoreThemeProvider>
    </DataContextProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  render(<App />, document.getElementById('mountPoint'))
}

document.addEventListener('DOMContentLoaded', initialize)
