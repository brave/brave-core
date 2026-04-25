/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'

// Components
import App from './components/app'
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'

// Utils
import store from './store'
import {
  getTorGeneralInfo,
  onGetTorGeneralInfo,
  onGetTorLog,
  onGetTorInitPercentage,
  onGetTorCircuitEstablished,
  onGetTorControlEvent
} from './slices/tor_internals.slice'

function initialize () {
  store.dispatch(getTorGeneralInfo())
  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <App />
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

// Expose functions to Page Handlers.
// TODO(petemill): Use event listeners instead.
// @ts-expect-error
window.tor_internals = {
  onGetTorGeneralInfo: (generalInfo: TorInternals.GeneralInfo) => {
    store.dispatch(onGetTorGeneralInfo({ generalInfo }))
  },
  onGetTorLog: (log: string) => {
    store.dispatch(onGetTorLog({ log }))
  },
  onGetTorInitPercentage: (percentage: string) => {
    store.dispatch(onGetTorInitPercentage({ percentage }))
  },
  onGetTorCircuitEstablished: (success: boolean) => {
    store.dispatch(onGetTorCircuitEstablished({ success }))
    store.dispatch(getTorGeneralInfo())
  },
  onGetTorControlEvent: (event: string) => {
    store.dispatch(onGetTorControlEvent({ event }))
  }
}

document.addEventListener('DOMContentLoaded', initialize)
