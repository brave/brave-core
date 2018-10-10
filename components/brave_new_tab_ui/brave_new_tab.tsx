/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'

import Theme from 'brave-ui/theme/brave-default'
import { ThemeProvider } from 'brave-ui/theme'

// Components
import App from './components/app'

// Utils
import store from './store'
import * as newTabActions from './actions/new_tab_actions'

window.cr.define('brave_new_tab', function () {
  'use strict'

  function initialize () {
    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function statsUpdated () {
    const actions = bindActionCreators(newTabActions, store.dispatch.bind(store))
    actions.statsUpdated()
  }

  return {
    initialize,
    statsUpdated
  }
})

document.addEventListener('DOMContentLoaded', window.brave_new_tab.initialize)
