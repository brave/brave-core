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

  function addresses (addresses: Record<Rewards.AddressesType, string>) {
    getActions().onAddresses(addresses)
  }

  function contributeList (list: Rewards.Publisher[]) {
    getActions().onContributeList(list)
  }

  function excludedNumber (num: number) {
    getActions().onExcludedNumber(num)
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

  function recurringTips (list: Rewards.Publisher[]) {
    getActions().onRecurringTips(list)
  }

  function currentTips (list: Rewards.Publisher[]) {
    getActions().onCurrentTips(list)
  }

  function initAutoContributeSettings (properties: any) {
    getActions().onInitAutoContributeSettings(properties)
  }

  function imported (imported: boolean) {
    getActions().onImportedCheck(imported)
  }

  function adsData (adsData: Rewards.AdsData) {
    getActions().onAdsData(adsData)
  }

  function pendingContributionTotal (amount: number) {
    getActions().onPendingContributionTotal(amount)
  }

  function onPendingContributionSaved (result: number) {
    if (result === 0) {
      getActions().getPendingContributionsTotal()
    }
  }

  function rewardsEnabled (enabled: boolean) {
    getActions().onRewardsEnabled(enabled)
  }

  function addressesForPaymentId (addresses: Record<Rewards.AddressesType, string>) {
    getActions().onAddressesForPaymentId(addresses)
  }

  function confirmationsHistory (totalPages: number, estimatedEarnings: number) {
    getActions().onConfirmationsHistory(totalPages, estimatedEarnings)
  }

  function confirmationsHistoryChanged () {
    getActions().onConfirmationsHistoryChanged()
  }

  function adsIsSupportedRegion (supported: boolean) {
    getActions().onAdsIsSupportedRegion(supported)
  }

  function recurringTipSaved (success: boolean) {
    getActions().onRecurringTipSaved(success)
  }

  function recurringTipRemoved (success: boolean) {
    getActions().onRecurringTipRemoved(success)
  }

  function onContributionSaved (properties: Rewards.ContributionSaved) {
    getActions().onContributionSaved(properties)
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
    excludedNumber,
    balanceReports,
    walletExists,
    contributionAmount,
    recurringTips,
    currentTips,
    initAutoContributeSettings,
    imported,
    adsData,
    pendingContributionTotal,
    onPendingContributionSaved,
    rewardsEnabled,
    addressesForPaymentId,
    confirmationsHistory,
    confirmationsHistoryChanged,
    adsIsSupportedRegion,
    recurringTipSaved,
    recurringTipRemoved,
    onContributionSaved
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards.initialize)
