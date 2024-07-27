// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { createRoot } from 'react-dom/client'
import { initLocale } from 'brave-ui'

import { loadTimeData } from '$web-common/loadTimeData'

import BraveCoreThemeProvider from '$web-common/BraveCoreThemeProvider'

import MainContainer from './main_container'
import DataContextProvider from './state/data-context-provider'

function App () {
  return (
    <DataContextProvider>
      <BraveCoreThemeProvider>
        <MainContainer />
      </BraveCoreThemeProvider>
    </DataContextProvider>
  )
}

function initialize () {
  initLocale(loadTimeData.data_)
  const root = createRoot(document.getElementById('root')!)
  root.render(<App />)
}

document.addEventListener('DOMContentLoaded', initialize)
