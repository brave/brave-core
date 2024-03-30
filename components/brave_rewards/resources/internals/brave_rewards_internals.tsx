/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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
import * as mojom from '../shared/lib/mojom'

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

function balance (balance: mojom.Balance) {
  getActions().onBalance(balance)
}

function contributions (contributions: RewardsInternals.ContributionInfo[]) {
  getActions().onContributions(contributions)
}

function partialLog (log: string) {
  getActions().onGetPartialLog(log)
}

function fullLog (log: string) {
  getActions().onGetFullLog(log)
}

function onGetExternalWallet (wallet: RewardsInternals.ExternalWallet) {
  getActions().onGetExternalWallet(wallet)
}

function eventLogs (logs: RewardsInternals.EventLog[]) {
  getActions().onEventLogs(logs)
}

function adDiagnostics (diagnostics: RewardsInternals.AdDiagnostics) {
  getActions().onAdDiagnostics(diagnostics)
}

function environment (environment: RewardsInternals.Environment) {
  getActions().onEnvironment(environment)
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
// @ts-expect-error
window.brave_rewards_internals = {
  onGetRewardsInternalsInfo,
  balance,
  contributions,
  partialLog,
  fullLog,
  onGetExternalWallet,
  eventLogs,
  adDiagnostics,
  environment
}

document.addEventListener('DOMContentLoaded', initialize)
