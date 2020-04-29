/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { bindActionCreators } from 'redux'
import { getActions as getUtilActions, setActions } from './utils'

// Components
import App from './components/app'

// Utils
import store from './store'
import * as rewardsInternalsActions from './actions/rewards_internals_actions'

window.cr.define('brave_rewards_internals', function () {
  'use strict'

  function getActions () {
    const actions: any = getUtilActions()
    if (actions) {
      return actions
    }

    const newActions = bindActionCreators(rewardsInternalsActions, store.dispatch.bind(store))
    setActions(newActions)
    return newActions
  }

  function onGetRewardsEnabled (enabled: boolean) {
    getActions().onGetRewardsEnabled(enabled)
  }

  function onGetRewardsInternalsInfo (info: RewardsInternals.State) {
    getActions().onGetRewardsInternalsInfo(info)
  }

  function balance (balance: RewardsInternals.Balance) {
    getActions().onBalance(balance)
  }

  function promotions (promotions: RewardsInternals.Promotion[]) {
    getActions().onPromotions(promotions)
  }

  function initialize () {
    window.i18nTemplate.process(window.document, window.loadTimeData)

    render(
      <Provider store={store}>
        <App />
      </Provider>,
      document.getElementById('root'))
  }

  return {
    initialize,
    onGetRewardsEnabled,
    onGetRewardsInternalsInfo,
    balance,
    promotions
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards_internals.initialize)
