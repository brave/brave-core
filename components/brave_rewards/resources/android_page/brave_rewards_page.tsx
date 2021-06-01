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
require('../../../../ui/webui/resources/fonts/muli.css')
require('../../../../ui/webui/resources/fonts/poppins.css')

import { WithThemeVariables } from '../shared/components/with_theme_variables'

// Utils
import { ThemeProvider } from 'styled-components'
import { loadTimeData } from '../../../common/loadTimeData'
import store from './store'
import Theme from 'brave-ui/theme/brave-default'
import { getActions as getUtilActions, getCurrentBalanceReport, setActions } from './utils'
import * as rewardsActions from './actions/rewards_actions'

function initialize () {
  initLocale(loadTimeData.data_)

  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <WithThemeVariables>
          <App />
        </WithThemeVariables>
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

function getActions () {
  const actions: any = getUtilActions()
  if (actions) {
    return actions
  }
  const newActions = bindActionCreators(rewardsActions, store.dispatch.bind(store))
  setActions(newActions)
  return newActions
}

function rewardsParameters (properties: Rewards.RewardsParameters) {
  getActions().onRewardsParameters(properties)
}

function promotions (properties: Rewards.PromotionResponse) {
  getActions().onPromotions(properties)
}

function promotionFinish (properties: Rewards.PromotionFinish) {
  getActions().onPromotionFinish(properties)
}

function reconcileStamp (stamp: number) {
  getActions().onReconcileStamp(stamp)
}

function contributeList (list: Rewards.Publisher[]) {
  getActions().onContributeList(list)
}

function excludedList (list: Rewards.ExcludedPublisher[]) {
  getActions().onExcludedList(list)
}

function balanceReport (properties: {month: number, year: number, report: Rewards.BalanceReport}) {
  getActions().onBalanceReport(properties)
}

function contributionAmount (amount: number) {
  getActions().onContributionAmount(amount)
}

function recurringTips (list: Rewards.Publisher[]) {
  getActions().onRecurringTips(list)
}

function currentTips (list: Rewards.Publisher[]) {
  getActions().onCurrentTips(list)
}

function autoContributeProperties (properties: any) {
  getActions().onAutoContributeProperties(properties)
}

function adsData (adsData: Rewards.AdsData) {
  getActions().onAdsData(adsData)
}

function onPendingContributionSaved (result: number) {
  if (result === 0) {
    getActions().getPendingContributions()
  }
}

function statement (data: any) {
  getActions().onStatement(data)
}

function statementChanged () {
  getActions().onStatementChanged()
}

function recurringTipSaved (success: boolean) {
  getActions().onRecurringTipSaved(success)
}

function recurringTipRemoved (success: boolean) {
  getActions().onRecurringTipRemoved(success)
}

function pendingContributions (list: Rewards.PendingContribution[]) {
  getActions().onPendingContributions(list)
}

function onRemovePendingContribution (result: number) {
  if (result === 0) {
    getActions().getPendingContributions()
  }
}

function excludedSiteChanged () {
  getActions().getExcludedSites()
  getActions().getContributeList()
}

function balance (properties: {status: number, balance: Rewards.Balance}) {
  getActions().onBalance(properties.status, properties.balance)
}

function reconcileComplete (properties: {type: number, result: number}) {
  chrome.send('brave_rewards.getReconcileStamp')
  getActions().getContributeList()
  getActions().getBalance()
  getActions().getRewardsParameters()
  getCurrentBalanceReport()

  if (properties.type === 8) { // Rewards.RewardsType.ONE_TIME_TIP
    chrome.send('brave_rewards.getOneTimeTips')
  }
}

function onlyAnonWallet (only: boolean) {
  getActions().onOnlyAnonWallet(only)
}

function unblindedTokensReady () {
  getActions().getBalance()
}

function initialized (result: number) {
  getActions().onInitialized(result)
}

// Expose functions to Page Handlers.
// TODO(petemill): Use event listeners instead.
// @ts-ignore
window.brave_rewards = {
  rewardsParameters,
  promotions,
  promotionFinish,
  // @ts-ignore
  reconcileStamp,
  // @ts-ignore
  contributeList,
  excludedList,
  balanceReport,
  contributionAmount,
  recurringTips,
  currentTips,
  autoContributeProperties,
  adsData,
  pendingContributions,
  onPendingContributionSaved,
  statement,
  statementChanged,
  recurringTipSaved,
  recurringTipRemoved,
  onRemovePendingContribution,
  excludedSiteChanged,
  balance,
  reconcileComplete,
  onlyAnonWallet,
  unblindedTokensReady,
  initialized
}

document.addEventListener('DOMContentLoaded', initialize)
