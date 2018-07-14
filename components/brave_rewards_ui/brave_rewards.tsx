/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
const { render } = require('react-dom')
const { Provider } = require('react-redux')
const App = require('./components/app')
const { bindActionCreators } = require('redux')

window.cr.define('brave_rewards', function () {
  'use strict'

  function initialize () {
    const store = require('./store')
    render(
      <Provider store={store}>
        <App />
      </Provider>,
      document.getElementById('root'))
    window.i18nTemplate.process(window.document, window.loadTimeData)
  }

  function getActions () {
    const store = require('./store')
    const rewardsActions = require('./actions/rewards_actions')
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
