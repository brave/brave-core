/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'

// Components
import App from './components/app'

// Utils
import store from './store'
import { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'
import { getActions as getUtilActions, setActions } from './helpers'
import * as syncActions from './actions/sync_actions'
import { bindActionCreators } from 'redux'

window.cr.define('sync_ui_exports', function () {
  'use strict'

  function initialize () {
    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root')
    )
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function getActions () {
    const actions: any = getUtilActions()
    if (actions) {
      return actions
    }
    const newActions = bindActionCreators(syncActions, store.dispatch.bind(store))
    setActions(newActions)
    return newActions
  }

  function showSettings (settings: any, devices: any) {
    getActions().onShowSettings(settings, devices)
  }

  function haveSyncWords (syncWords: string) {
    getActions().onHaveSyncWords(syncWords)
  }

  function haveSeedForQrCode (seed: string) {
    getActions().onHaveSeedForQrCode(seed)
  }

  // for testing purposes
  function logMessage (message: string) {
    getActions().onLogMessage(message)
  }

  return {
    initialize,
    showSettings,
    haveSyncWords,
    haveSeedForQrCode,
    logMessage
  }
})

document.addEventListener('DOMContentLoaded', window.sync_ui_exports.initialize)
