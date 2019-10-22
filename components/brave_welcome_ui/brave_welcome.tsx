/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { bindActionCreators } from 'redux'
import { Provider } from 'react-redux'

import welcomeDarkTheme from './theme/welcome-dark'
import welcomeLightTheme from './theme/welcome-light'
import BraveCoreThemeProvider from '../common/BraveCoreThemeProvider'

// Components
import App from './containers/app'

// Utils
import store from './store'
import * as welcomeActions from './actions/welcome_actions'

window.cr.define('brave_welcome', function () {
  'use strict'

  function loadWelcomeData () {
    const actions = bindActionCreators(welcomeActions, store.dispatch.bind(store))
    actions.getSearchEngineProviders()
    actions.getBrowserProfiles()
    actions.getBrowserThemes()
    chrome.braveRewards.onWalletInitialized.addListener((result: any) => {
      actions.onWalletInitialized(result)
    })
  }

  function initialize () {
    loadWelcomeData()
    new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
    .then((themeType: chrome.braveTheme.ThemeType) => {
      render(
        <Provider store={store}>
          <BraveCoreThemeProvider
            initialThemeType={themeType}
            dark={welcomeDarkTheme}
            light={welcomeLightTheme}
          >
            <App />
          </BraveCoreThemeProvider>
        </Provider>,
        document.getElementById('root'))
    })
    .catch((error) => {
      console.error('Problem mounting brave welcome', error)
    })
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  return {
    initialize
  }
})

document.addEventListener('DOMContentLoaded', window.brave_welcome.initialize)
