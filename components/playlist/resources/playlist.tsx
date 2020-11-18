/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'

// Components
import App from './components/app'

// Theme
import BraveCoreThemeProvider from '../../common/BraveCoreThemeProvider'
import Theme from 'brave-ui/theme/brave-default'
import DarkTheme from 'brave-ui/theme/brave-dark'
// Utils
import store from './store'

window.cr.define('playlist', function () {
  'use strict'

  function initialize () {
    new Promise(resolve => chrome.braveTheme.getBraveThemeType(resolve))
      .then((themeType: chrome.braveTheme.ThemeType) => {
        render(
          <Provider store={store}>
            <BraveCoreThemeProvider
              initialThemeType={themeType}
              dark={DarkTheme}
              light={Theme}
            >
              <App />
            </BraveCoreThemeProvider>
          </Provider>,
          document.getElementById('root'))
      })
      .catch((error) => {
        console.error('Problem mounting brave new tab', error)
      })
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  return {
    initialize
  }
})

document.addEventListener('DOMContentLoaded', window.playlist.initialize)
