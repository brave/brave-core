/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { bindActionCreators } from 'redux'
require('emptykit.css')

import { LocaleContext } from '../shared/lib/locale_context'
import { WithThemeVariables } from '../shared/components/with_theme_variables'

// Components
import App from './components/app'
require('../../../../ui/webui/resources/fonts/muli.css')
require('../../../../ui/webui/resources/fonts/poppins.css')

// Utils
import { ThemeProvider } from 'styled-components'
import { loadTimeData } from '../../../common/loadTimeData'
import store from './store'
import Theme from 'brave-ui/theme/brave-default'
import { getActions as getUtilActions, setActions, getCurrentBalanceReport } from './utils'
import * as rewardsActions from './actions/rewards_actions'

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
    getActions().setFirstLoad(false)
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

  // EXPIRED TOKEN
  if (properties.result === 24) {
    getActions().getExternalWallet()
  }
}

function externalWallet (properties: {result: number, wallet: Rewards.ExternalWallet}) {
  getActions().onExternalWallet(properties.result, properties.wallet)
}

function processRewardsPageUrl (data: Rewards.ProcessRewardsPageUrl) {
  getActions().onProcessRewardsPageUrl(data)
}

function disconnectWallet (properties: {result: number}) {
  if (properties.result === 0) {
    getActions().getExternalWallet()
    getActions().getBalance()
    return
  }
  getActions().disconnectWalletError()
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

function paymentId (paymentId: string) {
  getActions().onPaymentId(paymentId)
}

function walletPassphrase (passphrase: string) {
  getActions().onWalletPassphrase(passphrase)
}

function onboardingStatus (result: { showOnboarding: boolean }) {
  getActions().onOnboardingStatus(result.showOnboarding)
}

// Expose functions to Page Handlers.
// TODO(petemill): Use event listeners instead.
// @ts-ignore
window.brave_rewards = {
  rewardsParameters,
  promotions,
  claimPromotion,
  recoverWalletData,
  promotionFinish,
  reconcileStamp,
  contributeList,
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
  onToggleAdOptInAction,
  onToggleAdOptOutAction,
  onToggleSaveAd,
  onToggleFlagAd,
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
  onlyAnonWallet,
  unblindedTokensReady,
  monthlyReport,
  reconcileStampReset,
  monthlyReportIds,
  countryCode,
  initialized,
  completeReset,
  paymentId,
  walletPassphrase,
  onboardingStatus
}

document.addEventListener('DOMContentLoaded', initialize)
