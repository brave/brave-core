/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { initLocale } from 'brave-ui'
import { ThemeProvider } from 'styled-components'
import Theme from 'brave-ui/theme/brave-default'
import { setIconBasePath } from '@brave/leo/react/icon'
import { createStore, bindActionCreators } from 'redux'

import { loadTimeData } from '../../../common/loadTimeData'
import { LocaleContext } from '../shared/lib/locale_context'
import { createLocaleContextForWebUI } from '../shared/lib/webui_locale_context'
import { PlatformContext } from './lib/platform_context'
import { WithThemeVariables } from '../shared/components/with_theme_variables'
import { createReducer } from './reducers'
import { getCurrentBalanceReport } from './reducers/utils'
import * as rewardsActions from './actions/rewards_actions'

import { App } from './components/app'
import * as mojom from '../shared/lib/mojom'

import * as Rewards from './lib/types'

const store = createStore(createReducer())
const actions = bindActionCreators(rewardsActions, store.dispatch.bind(store))

setIconBasePath('chrome://resources/brave-icons')

function initialize () {
  // For `brave://rewards/reconnect`, automatically trigger reconnection on page
  // load and send the user immediately to the external wallet login page. Do
  // not show any additional UI while redirecting.
  if (window.location.pathname === '/reconnect') {
    chrome.send('brave_rewards.reconnectExternalWallet')
    return
  }

  initLocale(loadTimeData.data_)

  const platformInfo = {
    isAndroid: loadTimeData.getBoolean('isAndroid')
  }

  render(
    <Provider store={store}>
      <ThemeProvider theme={Theme}>
        <LocaleContext.Provider value={createLocaleContextForWebUI()}>
          <PlatformContext.Provider value={platformInfo}>
            <WithThemeVariables>
              <App />
            </WithThemeVariables>
          </PlatformContext.Provider>
        </LocaleContext.Provider>
      </ThemeProvider>
    </Provider>,
    document.getElementById('root'))
}

function userType (userType: number) {
  actions.onUserType(userType)
}

function isTermsOfServiceUpdateRequired (updateRequired: boolean) {
  actions.onIsTermsOfServiceUpdateRequired(updateRequired)
}

function rewardsParameters (properties: Rewards.RewardsParameters) {
  actions.onRewardsParameters(properties)
  // Get the current AC amount after rewards parameters have been
  // updated, as the default AC amount may have been changed.
  actions.getContributionAmount()
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

function balanceReport (properties: { month: number, year: number, report: Rewards.BalanceReport }) {
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

function onIsAutoContributeSupported (isAcSupported: boolean) {
  actions.onIsAutoContributeSupported(isAcSupported)
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

function onToggleAdThumbUp(success: boolean) {
  actions.onToggleAdThumbUp(success)
}

function onToggleAdThumbDown(success: boolean) {
  actions.onToggleAdThumbDown(success)
}

function onToggleAdOptIn(success: boolean) {
  actions.onToggleAdOptIn(success)
}

function onToggleAdOptOut(success: boolean) {
  actions.onToggleAdOptOut(success)
}

function onToggleSavedAd(success: boolean) {
  actions.onToggleSavedAd(success)
}

function onToggleFlaggedAd(success: boolean) {
  actions.onToggleFlaggedAd(success)
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

function excludedSiteChanged () {
  actions.getExcludedSites()
  actions.getContributeList()
}

function balance (balance?: mojom.Balance) {
  actions.onBalance(balance)
}

function reconcileComplete (properties: { type: number, result: number }) {
  chrome.send('brave_rewards.getReconcileStamp')
  actions.getContributeList()
  actions.getBalance()
  actions.getRewardsParameters()
  actions.getTipTable()

  getCurrentBalanceReport()

  // EXPIRED TOKEN
  if (properties.result === 24) {
    actions.getExternalWallet()
  }
}

function onGetExternalWallet (externalWallet?: mojom.ExternalWallet) {
  actions.onGetExternalWallet(externalWallet)
}

function onConnectExternalWallet (result: mojom.ConnectExternalWalletResult) {
  actions.onConnectExternalWallet(result)
}

function onExternalWalletLoggedOut () {
  actions.getExternalWallet()
  actions.getBalance()
}

function onExternalWalletDisconnected () {
  actions.getUserType()
}

function reconcileStampReset () {
  actions.onReconcileStampReset()
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

function onboardingStatus (result: { showOnboarding: boolean }) {
  actions.onOnboardingStatus(result.showOnboarding)
}

function externalWalletLogin (url: string) {
  if (url) {
    window.open(url, '_self', 'noreferrer')
  } else {
    actions.onExternalWalletLoginError()
  }
}

function onPrefChanged (key: string) {
  actions.onPrefChanged(key)
}

function onIsUnsupportedRegion (isUnsupportedRegion: boolean) {
  actions.onIsUnsupportedRegion(isUnsupportedRegion)
}

// Expose functions to Page Handlers.
Object.defineProperty(window, 'brave_rewards', {
  configurable: true,
  value: {
    userType,
    isTermsOfServiceUpdateRequired,
    rewardsParameters,
    reconcileStamp,
    contributeList,
    externalWalletProviderList,
    excludedList,
    balanceReport,
    contributionAmount,
    recurringTips,
    currentTips,
    onIsAutoContributeSupported,
    autoContributeProperties,
    adsData,
    adsHistory,
    onToggleAdThumbUp,
    onToggleAdThumbDown,
    onToggleAdOptIn,
    onToggleAdOptOut,
    onToggleSavedAd,
    onToggleFlaggedAd,
    statement,
    statementChanged,
    recurringTipSaved,
    recurringTipRemoved,
    excludedSiteChanged,
    balance,
    reconcileComplete,
    onGetExternalWallet,
    onConnectExternalWallet,
    onExternalWalletLoggedOut,
    onExternalWalletDisconnected,
    reconcileStampReset,
    countryCode,
    initialized,
    completeReset,
    onboardingStatus,
    externalWalletLogin,
    onPrefChanged,
    onIsUnsupportedRegion
  }
})

document.addEventListener('DOMContentLoaded', initialize)
