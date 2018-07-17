/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'
import { initLocale } from 'brave-ui'

// Components
import App from './components/app'

// Utils
import store from './store'
import * as rewardsActions from './actions/rewards_actions'

window.cr.define('brave_rewards', function () {
  'use strict'

  function initialize () {
    window.i18nTemplate.process(window.document, window.loadTimeData)
    if (window.loadTimeData && window.loadTimeData.data_) {
      initLocale(window.loadTimeData.data_)
    }

    render(
      <Provider store={store}>
        <App />
      </Provider>,
      document.getElementById('root'))
  }

  function getActions () {
    return bindActionCreators(rewardsActions, store.dispatch.bind(store))
  }

  function walletCreated () {
    getActions().walletCreated()
  }

  function walletCreateFailed () {
    getActions().walletCreateFailed()
  }

  return {
    initialize,
    walletCreated,
    walletCreateFailed
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards.initialize)
