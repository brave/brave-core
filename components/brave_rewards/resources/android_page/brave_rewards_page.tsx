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
import { getActions as getUtilActions, setActions } from './utils'
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
        <ThemeProvider theme={Theme}>
          <App />
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

  function walletCreated () {
    getActions().onWalletCreated()
  }

  function walletCreateFailed () {
    getActions().onWalletCreateFailed()
  }

  function walletProperties (properties: {status: number, wallet: Rewards.WalletProperties, monthlyAmount: number}) {
    getActions().onWalletProperties(properties)
  }

  function grant (properties: Rewards.GrantResponse) {
    getActions().onGrant(properties)
  }

  function grantCaptcha (captcha: Rewards.Captcha) {
    getActions().onGrantCaptcha(captcha)
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

  function addresses (addresses: Record<string, string>) {
    getActions().onAddresses(addresses)
  }

  function contributeList (list: Rewards.Publisher[]) {
    getActions().onContributeList(list)
  }

  function excludedList (list: Rewards.ExcludedPublisher[]) {
    getActions().onExcludedList(list)
  }

  function excludedSiteChanged () {
    getActions().getExcludedSites()
    getActions().getContributeList()
  }

  function balanceReports (reports: Record<string, Rewards.Report>) {
    getActions().onBalanceReports(reports)
  }

  function walletExists (exists: boolean) {
    getActions().onWalletExists(exists)
  }

  function contributionAmount (amount: number) {
    getActions().onContributionAmount(amount)
  }

  function adsData (adsData: Rewards.AdsData) {
    getActions().onAdsData(adsData)
  }

  function initAutoContributeSettings (properties: any) {
    getActions().onInitAutoContributeSettings(properties)
  }

  function recurringTips (list: Rewards.Publisher[]) {
    getActions().onRecurringDonationUpdate(list)
  }

  function currentTips (list: Rewards.Publisher[]) {
    getActions().onCurrentTips(list)
  }

  function rewardsEnabled (enabled: boolean) {
    getActions().onRewardsEnabled(enabled)
  }

  function pendingContributionTotal (amount: number) {
    getActions().onPendingContributionTotal(amount)
  }

  function onPendingContributionSaved (result: number) {
    if (result === 0) {
      getActions().getPendingContributionsTotal()
    }
  }

  function recurringTipRemoved (success: boolean) {
    getActions().onRecurringTipRemoved(success)
  }

  function transactionHistory (data: {adsEstimatedPendingRewards: number, adsNextPaymentDate: string, adsNotificationsReceivedThisMonth: number}) {
    getActions().onTransactionHistory(data)
  }

  function transactionHistoryChanged () {
    getActions().onTransactionHistoryChanged()
  }

  function balance (properties: {status: number, balance: Rewards.Balance}) {
    getActions().onBalance(properties.status, properties.balance)
  }

  function reconcileComplete (properties: {category: number, result: number}) {
    chrome.send('brave_rewards.getReconcileStamp')
    getActions().getContributeList()
    getActions().getBalance()
    getActions().getWalletProperties()
  }

  function onlyAnonWallet (only: boolean) {
    getActions().onOnlyAnonWallet(only)
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
    reconcileStamp,
    addresses,
    contributeList,
    balanceReports,
    walletExists,
    contributionAmount,
    adsData,
    initAutoContributeSettings,
    recurringTips,
    currentTips,
    rewardsEnabled,
    pendingContributionTotal,
    recurringTipRemoved,
    onPendingContributionSaved,
    transactionHistory,
    transactionHistoryChanged,
    excludedList,
    excludedSiteChanged,
    balance,
    reconcileComplete,
    onlyAnonWallet
  }
})

if (document.readyState === 'complete'
    || document.readyState === 'interactive') {
  // document has at least been parsed
  window.brave_rewards.initialize()
} else {
  document.addEventListener('DOMContentLoaded', window.brave_rewards.initialize)
}
