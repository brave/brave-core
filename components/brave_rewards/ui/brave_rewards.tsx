/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'
import { initLocale } from 'brave-ui'
require('emptykit.css')

// Components
import App from './components/app'
require('../../fonts/muli.css')
require('../../fonts/poppins.css')

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
    getActions().onWalletCreated()
  }

  function walletCreateFailed () {
    getActions().onWalletCreateFailed()
  }

  function walletProperties (properties: {status: number, wallet: Rewards.WalletProperties}) {
    getActions().onWalletProperties(properties)
  }

  function grant (properties: Rewards.Grant) {
    getActions().onGrant(properties)
  }

  function grantCaptcha (image: string) {
    getActions().onGrantCaptcha(image)
  }

  function walletPassphrase (pass: string) {
    getActions().onWalletPassphrase(pass)
  }

  function recoverWalletData (properties: Rewards.RecoverWallet) {
    getActions().onRecoverWalletData(properties)
  }

  function grantFinish (properties: Rewards.GrantFinish) {
    getActions().onGrantFinish(properties)
  }

  function reconcileStamp (stamp: number) {
    getActions().onReconcileStamp(stamp)
  }

  return {
    initialize,
    walletCreated,
    walletCreateFailed,
    walletProperties,
    grant,
    grantCaptcha,
    walletPassphrase,
    recoverWalletData,
    grantFinish,
    reconcileStamp
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards.initialize)
