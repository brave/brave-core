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
// import { getActions as getUtilActions, setActions } from './utils'
// import * as syncActions from './actions/sync_actions'

window.cr.define('sync_ui_exports', function () {
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

  // TODO: Check whether or not we need this for sync actions API
  // function getActions () {
  //   const actions: any = getUtilActions()
  //   if (actions) {
  //     return actions
  //   }
  //   const newActions = bindActionCreators(rewardsActions, store.dispatch.bind(store))
  //   setActions(newActions)
  //   return newActions
  // }

  return {
    initialize
  }
})

console.log('-------------- sync_ui_exports: ', window.sync_ui_exports)
document.addEventListener('DOMContentLoaded', window.sync_ui_exports.initialize)
