/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { bindActionCreators } from 'redux'
require('emptykit.css')

// Components
import App from './components/app'
require('../../../fonts/muli.css')
require('../../../fonts/poppins.css')

// Utils
import store from './store'
import { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'
import * as rewardsActions from './actions/donate_actions'

let actions: any

window.cr.define('brave_rewards_donate', function () {
  'use strict'

  function initialize () {
    window.i18nTemplate.process(window.document, window.loadTimeData)
    if (window.loadTimeData && window.loadTimeData.data_) {
      initLocale(window.loadTimeData.data_)
    }

    render(
      <Provider store={store}>
        <ThemeProvider theme={Theme}>
          <App />
        </ThemeProvider>
      </Provider>,
      document.getElementById('root'))

    // TODO call rewards service to get dialog data
    const dialogArgsRaw = chrome.getVariableValue('dialogArguments')
    try {
      const args = JSON.parse(dialogArgsRaw)
      chrome.send('brave_rewards_donate.getPublisherBanner', [args.publisherKey])
    } catch (e) {
      console.error('Error parsing incoming dialog args', dialogArgsRaw, e)
    }
  }

  function getActions () {
    if (actions) {
      return actions
    }

    actions = bindActionCreators(rewardsActions, store.dispatch.bind(store))
    return actions
  }

  function publisherBanner (data: RewardsDonate.Publisher) {
    getActions().onPublisherBanner(data)
  }

  function walletProperties (properties: {status: number, wallet: RewardsDonate.WalletProperties}) {
    getActions().onWalletProperties(properties)
  }

  function recurringDonations (list: string[]) {
    getActions().onRecurringDonations(list)
  }

  return {
    initialize,
    publisherBanner,
    walletProperties,
    recurringDonations
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards_donate.initialize)
