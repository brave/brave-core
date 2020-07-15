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

// Utils
import store from './store'
import { ThemeProvider } from 'brave-ui/theme'
import Theme from 'brave-ui/theme/brave-default'
import { getActions as getUtilActions, setActions, getCurrentBalanceReport } from './utils'
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

  function rewardsParameters (properties: Rewards.RewardsParameters) {
    getActions().onRewardsParameters(properties)
    // Get the current AC amount after rewards parameters have been
    // updated, as the default AC amount may have been changed.
    getActions().getContributionAmount()
  }

  function promotions (properties: Rewards.PromotionResponse) {
    getActions().onPromotions(properties)
  }

  function claimPromotion (properties: Rewards.Captcha) {
    getActions().onClaimPromotion(properties)
  }

  function walletPassphrase (pass: string) {
    getActions().onWalletPassphrase(pass)
  }

  function recoverWalletData (result: number) {
    getActions().onRecoverWalletData(result)
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

  function autoContributeProperties (properties: any) {
    getActions().onAutoContributeProperties(properties)
  }

  function adsData (adsData: Rewards.AdsData) {
    getActions().onAdsData(adsData)
  }

  function adsHistory (adsHistory: Rewards.AdsHistory[]) {
    getActions().onAdsHistory(adsHistory)
  }

  function onToggleAdThumbUp (result: Rewards.ToggleLikeAction) {
    getActions().onToggleAdThumbUp(result)
  }

  function onToggleAdThumbDown (result: Rewards.ToggleLikeAction) {
    getActions().onToggleAdThumbDown(result)
  }

  function onToggleAdOptInAction (result: Rewards.ToggleOptAction) {
    getActions().onToggleAdOptInAction(result)
  }

  function onToggleAdOptOutAction (result: Rewards.ToggleOptAction) {
    getActions().onToggleAdOptOutAction(result)
  }

  function onToggleSaveAd (result: Rewards.ToggleSaveAd) {
    getActions().onToggleSaveAd(result)
  }

  function onToggleFlagAd (result: Rewards.ToggleFlagAd) {
    getActions().onToggleFlagAd(result)
  }

  function onPendingContributionSaved (result: number) {
    if (result === 0) {
      getActions().getPendingContributions()
    }
  }

  function rewardsEnabled (enabled: boolean) {
    getActions().onRewardsEnabled(enabled)
  }

  function transactionHistory (data: {adsEstimatedPendingRewards: number, adsNextPaymentDate: string, adsNotificationsReceivedThisMonth: number}) {
    getActions().onTransactionHistory(data)
  }

  function transactionHistoryChanged () {
    getActions().onTransactionHistoryChanged()
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

    // EXPIRED TOKEN
    if (properties.result === 24) {
      getActions().getExternalWallet('uphold')
    }
  }

  function externalWallet (properties: {result: number, wallet: Rewards.ExternalWallet}) {
    getActions().onExternalWallet(properties.result, properties.wallet)
  }

  function processRewardsPageUrl (data: Rewards.ProcessRewardsPageUrl) {
    getActions().onProcessRewardsPageUrl(data)
  }

  function disconnectWallet (properties: {walletType: string, result: number}) {
    if (properties.result === 0) {
      getActions().getExternalWallet(properties.walletType)
      getActions().getBalance()
    }
  }

  function onlyAnonWallet (only: boolean) {
    getActions().onOnlyAnonWallet(only)
  }

  function unblindedTokensReady () {
    getActions().getBalance()
  }

  function monthlyReport (properties: { result: number, month: number, year: number, report: Rewards.MonthlyReport}) {
    getActions().onMonthlyReport(properties)
  }

  function reconcileStampReset () {
    getActions().onReconcileStampReset()
  }

  function monthlyReportIds (ids: string[]) {
    getActions().onMonthlyReportIds(ids)
  }

  function countryCode (countryCode: string) {
    getActions().onCountryCode(countryCode)
  }

  function initialized (result: number) {
    getActions().onInitialized(result)
  }

  function completeReset (success: boolean) {
    getActions().onCompleteReset(success)
  }

  return {
    initialize,
    walletCreated,
    walletCreateFailed,
    rewardsParameters,
    promotions,
    claimPromotion,
    walletPassphrase,
    recoverWalletData,
    promotionFinish,
    reconcileStamp,
    contributeList,
    excludedList,
    balanceReport,
    walletExists,
    contributionAmount,
    recurringTips,
    currentTips,
    autoContributeProperties,
    adsData,
    adsHistory,
    onToggleAdThumbUp,
    onToggleAdThumbDown,
    onToggleAdOptInAction,
    onToggleAdOptOutAction,
    onToggleSaveAd,
    onToggleFlagAd,
    pendingContributions,
    onPendingContributionSaved,
    rewardsEnabled,
    transactionHistory,
    transactionHistoryChanged,
    recurringTipSaved,
    recurringTipRemoved,
    onRemovePendingContribution,
    excludedSiteChanged,
    balance,
    reconcileComplete,
    externalWallet,
    processRewardsPageUrl,
    disconnectWallet,
    onlyAnonWallet,
    unblindedTokensReady,
    monthlyReport,
    reconcileStampReset,
    monthlyReportIds,
    countryCode,
    initialized,
    completeReset
  }
})

document.addEventListener('DOMContentLoaded', window.brave_rewards.initialize)
