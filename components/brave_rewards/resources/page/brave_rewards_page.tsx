/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'
import { createStore, bindActionCreators } from 'redux'

import { loadTimeData } from '../../../common/loadTimeData'
import { LocaleContext } from '../shared/lib/locale_context'
import { WithThemeVariables } from '../shared/components/with_theme_variables'
import { createReducer } from './reducers'
import { getCurrentBalanceReport } from './reducers/utils'
import * as rewardsActions from './actions/rewards_actions'

import { App } from './components/app'

const store = createStore(createReducer())
const actions = bindActionCreators(rewardsActions, store.dispatch.bind(store))

function initialize () {
  initLocale(loadTimeData.data_)

  const localeContext = {
    getString: (key: string) => loadTimeData.getString(key)
  }

  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <LocaleContext.Provider value={localeContext}>
          <WithThemeVariables>
            <App />
          </WithThemeVariables>
        </LocaleContext.Provider>
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

function rewardsParameters (properties: Rewards.RewardsParameters) {
  actions.onRewardsParameters(properties)
  // Get the current AC amount after rewards parameters have been
  // updated, as the default AC amount may have been changed.
  actions.getContributionAmount()
}

function promotions (properties: Rewards.PromotionResponse) {
  actions.onPromotions(properties)
}

function recoverWalletData (result: number) {
  actions.onRecoverWalletData(result)
}

function promotionFinish (properties: Rewards.PromotionFinish) {
  actions.onPromotionFinish(properties)
}

function reconcileStamp (stamp: number) {
  actions.onReconcileStamp(stamp)
}

function contributeList (list: Rewards.Publisher[]) {
  actions.onContributeList(list)
}

function excludedList (list: Rewards.ExcludedPublisher[]) {
  actions.onExcludedList(list)
}

function balanceReport (properties: {month: number, year: number, report: Rewards.BalanceReport}) {
  actions.onBalanceReport(properties)
}

function contributionAmount (amount: number) {
  actions.onContributionAmount(amount)
}

function recurringTips (list: Rewards.Publisher[]) {
  actions.onRecurringTips(list)
}

function currentTips (list: Rewards.Publisher[]) {
  actions.onCurrentTips(list)
}

function autoContributeProperties (properties: any) {
  actions.onAutoContributeProperties(properties)
}

function adsData (adsData: Rewards.AdsData) {
  actions.onAdsData(adsData)
}

function adsHistory (adsHistory: Rewards.AdsHistory[]) {
  actions.onAdsHistory(adsHistory)
}

function onToggleAdThumbUp (result: Rewards.ToggleLikeAction) {
  actions.onToggleAdThumbUp(result)
}

function onToggleAdThumbDown (result: Rewards.ToggleLikeAction) {
  actions.onToggleAdThumbDown(result)
}

function onToggleAdOptIn (result: Rewards.ToggleOptAction) {
  actions.onToggleAdOptIn(result)
}

function onToggleAdOptOut (result: Rewards.ToggleOptAction) {
  actions.onToggleAdOptOut(result)
}

function onToggleSavedAd (result: Rewards.ToggleSavedAd) {
  actions.onToggleSavedAd(result)
}

function onToggleFlaggedAd (result: Rewards.ToggleFlaggedAd) {
  actions.onToggleFlaggedAd(result)
}

function onPendingContributionSaved (result: number) {
  if (result === 0) {
    actions.getPendingContributions()
  }
}

function statement (data: any) {
  actions.onStatement(data)
}

function statementChanged () {
  actions.onStatementChanged()
}

function recurringTipSaved (success: boolean) {
  actions.onRecurringTipSaved(success)
}

function recurringTipRemoved (success: boolean) {
  actions.onRecurringTipRemoved(success)
}

function externalWalletProviderList (list: Rewards.ExternalWalletProvider[]) {
  actions.onExternalWalletProviderList(list)
}

function pendingContributions (list: Rewards.PendingContribution[]) {
  actions.onPendingContributions(list)
}

function onRemovePendingContribution (result: number) {
  if (result === 0) {
    actions.getPendingContributions()
  }
}

function excludedSiteChanged () {
  actions.getExcludedSites()
  actions.getContributeList()
}

function balance (properties: {status: number, balance: Rewards.Balance}) {
  actions.onBalance(properties.status, properties.balance)
}

function reconcileComplete (properties: {type: number, result: number}) {
  chrome.send('brave_rewards.getReconcileStamp')
  actions.getContributeList()
  actions.getBalance()
  actions.getRewardsParameters()

  getCurrentBalanceReport()

  if (properties.type === 8) { // Rewards.RewardsType.ONE_TIME_TIP
    chrome.send('brave_rewards.getOneTimeTips')
  }

  // EXPIRED TOKEN
  if (properties.result === 24) {
    actions.getExternalWallet()
  }
}

function externalWallet (properties: {result: number, wallet: Rewards.ExternalWallet}) {
  actions.onExternalWallet(properties.result, properties.wallet)
}

function processRewardsPageUrl (data: Rewards.ProcessRewardsPageUrl) {
  actions.onProcessRewardsPageUrl(data)
}

function disconnectWallet (properties: {result: number}) {
  if (properties.result === 0) {
    actions.getExternalWallet()
    actions.getBalance()
    return
  }
  actions.disconnectWalletError()
}

function unblindedTokensReady () {
  actions.getBalance()
}

function monthlyReport (properties: { result: number, month: number, year: number, report: Rewards.MonthlyReport}) {
  actions.onMonthlyReport(properties)
}

function reconcileStampReset () {
  actions.onReconcileStampReset()
}

function monthlyReportIds (ids: string[]) {
  actions.onMonthlyReportIds(ids)
}

function countryCode (countryCode: string) {
  actions.onCountryCode(countryCode)
}

function initialized () {
  actions.onInitialized()
}

function completeReset (success: boolean) {
  actions.onCompleteReset(success)
}

function paymentId (paymentId: string) {
  actions.onPaymentId(paymentId)
}

function onboardingStatus (result: { showOnboarding: boolean }) {
  actions.onOnboardingStatus(result.showOnboarding)
}

function enabledInlineTippingPlatforms (list: string[]) {
  actions.onEnabledInlineTippingPlatforms(list)
}

function externalWalletLogin (url: string) {
  window.open(url, '_self')
}

function onPrefChanged (key: string) {
  actions.onPrefChanged(key)
}

// Expose functions to Page Handlers.
Object.defineProperty(window, 'brave_rewards', {
  configurable: true,
  value: {
    rewardsParameters,
    promotions,
    recoverWalletData,
    promotionFinish,
    reconcileStamp,
    contributeList,
    externalWalletProviderList,
    excludedList,
    balanceReport,
    contributionAmount,
    recurringTips,
    currentTips,
    autoContributeProperties,
    adsData,
    adsHistory,
    onToggleAdThumbUp,
    onToggleAdThumbDown,
    onToggleAdOptIn,
    onToggleAdOptOut,
    onToggleSavedAd,
    onToggleFlaggedAd,
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
    externalWallet,
    processRewardsPageUrl,
    disconnectWallet,
    unblindedTokensReady,
    monthlyReport,
    reconcileStampReset,
    monthlyReportIds,
    countryCode,
    initialized,
    completeReset,
    paymentId,
    onboardingStatus,
    enabledInlineTippingPlatforms,
    externalWalletLogin,
    onPrefChanged
  }
})

document.addEventListener('DOMContentLoaded', initialize)
