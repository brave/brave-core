/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'

// Containers
import App from './containers/App'

// Utils
import store from './store'
import * as webcompatReporterActions from './actions/webcompatreporter_actions'

let actions: any

function getActions () {
  if (actions) {
    return actions
  }

  actions = bindActionCreators(webcompatReporterActions, store.dispatch.bind(store))
  return actions
}

function loadDialogArgs () {
  const dialogArgsRaw = chrome.getVariableValue('dialogArguments')
  let dialogArgs
  try {
    dialogArgs = JSON.parse(dialogArgsRaw)
  } catch (e) {
    console.error('Error parsing incoming dialog args: ', dialogArgsRaw, e)
  }

  getActions().setSiteUrl(dialogArgs.siteUrl)
}

function initialize () {
  loadDialogArgs()

  render(
    <Provider store={store}>
      <BraveCoreThemeProvider>
        <App />
      </BraveCoreThemeProvider>
    </Provider>,
    document.getElementById('root')
  )
}

document.addEventListener('DOMContentLoaded', initialize)
