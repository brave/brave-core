/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { bindActionCreators } from 'redux'
import { Provider } from 'react-redux'

import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'

// Components
import App from './containers/app'

// Utils
import store from './store'
import * as welcomeActions from './actions/welcome_actions'

window.cr.define('brave_welcome', function () {
  'use strict'

  function getSearchEngineProviders () {
    const actions = bindActionCreators(welcomeActions, store.dispatch.bind(store))
    actions.getSearchEngineProviders()
  }

  function getBrowserProfiles () {
    const actions = bindActionCreators(welcomeActions, store.dispatch.bind(store))
    actions.getBrowserProfiles()
  }

  function initialize () {
    getSearchEngineProviders()
    getBrowserProfiles()
    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  return {
    initialize
  }
})

document.addEventListener('DOMContentLoaded', window.brave_welcome.initialize)
