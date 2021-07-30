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
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'

// Utils
import store from './store'
import * as rewardsInternalsActions from './actions/rewards_internals_actions'

function getActions () {
  const actions: any = getUtilActions()
  if (actions) {
    return actions
  }

  const newActions = bindActionCreators(rewardsInternalsActions, store.dispatch.bind(store))
  setActions(newActions)
  return newActions
}

function onGetRewardsInternalsInfo (info: RewardsInternals.State) {
  getActions().onGetRewardsInternalsInfo(info)
}

function balance (balance: RewardsInternals.Balance) {
  getActions().onBalance(balance)
}

function contributions (contributions: RewardsInternals.ContributionInfo[]) {
  getActions().onContributions(contributions)
}

function promotions (promotions: RewardsInternals.Promotion[]) {
  getActions().onPromotions(promotions)
}

function partialLog (log: string) {
  getActions().onGetPartialLog(log)
}

function fullLog (log: string) {
  getActions().onGetFullLog(log)
}

function externalWallet (properties: {result: number, wallet: RewardsInternals.ExternalWallet}) {
  getActions().onExternalWallet(properties.result, properties.wallet)
}

function eventLogs (logs: RewardsInternals.EventLog[]) {
  getActions().onEventLogs(logs)
}

function initialize () {
  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <App />
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

// Expose functions to Page Handlers.
// TODO(petemill): Use event listeners instead.
// @ts-ignore
window.brave_rewards_internals = {
  // @ts-ignore
  onGetRewardsInternalsInfo,
  balance,
  contributions,
  promotions,
  partialLog,
  fullLog,
  externalWallet,
  eventLogs
}

document.addEventListener('DOMContentLoaded', initialize)
