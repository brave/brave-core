/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

// Components
import App from './components/app'
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'

// Utils
import store from './store'
import * as torInternalsActions from './actions/tor_internals_actions'

function getTorGeneralInfo () {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.getTorGeneralInfo()
}

function onGetTorGeneralInfo (generalInfo: TorInternals.GeneralInfo) {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.onGetTorGeneralInfo(generalInfo)
}

function onGetTorLog (log: string) {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.onGetTorLog(log)
}

function onGetTorInitPercentage (percentage: string) {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.onGetTorInitPercentage(percentage)
}

function onGetTorCircuitEstablished (success: boolean) {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.onGetTorCircuitEstablished(success)
  getTorGeneralInfo()
}

function onGetTorControlEvent (event: string) {
  const actions = bindActionCreators(torInternalsActions, store.dispatch.bind(store))
  actions.onGetTorControlEvent(event)
}

function initialize () {
  getTorGeneralInfo()
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
// @ts-ignore
window.tor_internals = {
  onGetTorGeneralInfo,
  onGetTorLog,
  onGetTorInitPercentage,
  onGetTorCircuitEstablished,
  onGetTorControlEvent
}

document.addEventListener('DOMContentLoaded', initialize)
