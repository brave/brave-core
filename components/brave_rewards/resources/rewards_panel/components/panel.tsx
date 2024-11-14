/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { shouldShowSelfCustodyInvite } from '../lib/derived_state'
import { TabOpenerContext } from '../../shared/components/new_tab_link'
import { OnboardingResult, RewardsOptIn } from '../../shared/components/onboarding'
import { WalletCard } from '../../shared/components/wallet_card'
import { getProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { LimitedView } from './limited_view'
import { NavBar } from './navbar'
import { PanelOverlays } from './panel_overlays'
import { PublisherCard } from './publisher_card'
import { SelfCustodyInvite } from './self_custody_invite'
import { TosUpdateNotice } from '../../shared/components/tos_update_notice'

import * as urls from '../../shared/lib/rewards_urls'

import * as style from './panel.style'

type ActiveView = 'tip' | 'summary'

export function Panel () {
  const host = React.useContext(HostContext)
  const tabOpener = React.useContext(TabOpenerContext)

  const [userType, setUserType] = React.useState(host.state.userType)
  const [balance, setBalance] = React.useState(host.state.balance)
  const [externalWallet, setExternalWallet] =
    React.useState(host.state.externalWallet)
  const [exchangeInfo, setExchangeInfo] =
    React.useState(host.state.exchangeInfo)
  const [earningsInfo, setEarningsInfo] =
    React.useState(host.state.earningsInfo)
  const [payoutStatus, setPayoutStatus] =
    React.useState(host.state.payoutStatus)
  const [summaryData, setSummaryData] = React.useState(host.state.summaryData)
  const [publisherInfo, setPublisherInfo] =
    React.useState(host.state.publisherInfo)

  const [activeView, setActiveView] = React.useState<ActiveView>(
    publisherInfo ? 'tip' : 'summary')

  const [requestedView, setRequestedView] =
    React.useState(host.state.requestedView)
  const [availableCountries, setAvailableCountries] =
    React.useState(host.state.availableCountries)
  const [defaultCountry, setDefaultCountry] =
    React.useState(host.state.defaultCountry)
  const [onboardingResult, setOnboardingResult] =
    React.useState<OnboardingResult | null>(null)
  const [rewardsEnabled, setRewardsEnabled] =
    React.useState(host.state.rewardsEnabled)
  const [declaredCountry, setDeclaredCountry] =
    React.useState(host.state.declaredCountry)
  const [showSelfCustodyInvite, setShowSelfCustodyInvite] =
    React.useState(shouldShowSelfCustodyInvite(host.state))
  const [tosUpdateRequired, setTosUpdateRequired] =
    React.useState(host.state.isTermsOfServiceUpdateRequired)

  useHostListener(host, (state) => {
    setUserType(state.userType)
    setBalance(state.balance)
    setExternalWallet(state.externalWallet)
    setExchangeInfo(state.exchangeInfo)
    setEarningsInfo(state.earningsInfo)
    setPayoutStatus(state.payoutStatus)
    setSummaryData(state.summaryData)
    setPublisherInfo(state.publisherInfo)

    setRequestedView(state.requestedView)
    setRewardsEnabled(state.rewardsEnabled)
    setDeclaredCountry(state.declaredCountry)
    setAvailableCountries(state.availableCountries)
    setDefaultCountry(state.defaultCountry)
    setShowSelfCustodyInvite(shouldShowSelfCustodyInvite(state))
    setTosUpdateRequired(state.isTermsOfServiceUpdateRequired)
  })

  const needsCountry = rewardsEnabled && !declaredCountry

  function renderOnboaring () {
    const onHideResult = () => {
      if (onboardingResult === 'success') {
        tabOpener.openTab(urls.rewardsTourURL)
      }
      setOnboardingResult(null)
      host.closePanel()
    }

    const onEnable = (country: string) => {
      host.enableRewards(country).then((result) => {
        setOnboardingResult(result)
      })
    }

    return (
      <RewardsOptIn
        availableCountries={availableCountries}
        defaultCountry={defaultCountry}
        initialView={needsCountry || requestedView === 'rewards-setup' ?
          'declare-country' : 'default'}
        result={onboardingResult}
        onEnable={onEnable}
        onHideResult={onHideResult}
      />
    )
  }

  const walletProvider = externalWallet ? externalWallet.provider : null

  const providerPayoutStatus = getProviderPayoutStatus(
    payoutStatus, walletProvider)

  function renderFull () {
    const onSettingsClick = () => host.openRewardsSettings()
    return (
      <>
        <WalletCard
          userType={userType}
          balance={balance}
          externalWallet={externalWallet}
          providerPayoutStatus={providerPayoutStatus}
          minEarningsThisMonth={earningsInfo.minEarningsThisMonth}
          maxEarningsThisMonth={earningsInfo.maxEarningsThisMonth}
          minEarningsLastMonth={earningsInfo.minEarningsLastMonth}
          maxEarningsLastMonth={earningsInfo.maxEarningsLastMonth}
          nextPaymentDate={earningsInfo.nextPaymentDate}
          exchangeRate={exchangeInfo.rate}
          exchangeCurrency={exchangeInfo.currency}
          showSummary={activeView === 'summary'}
          summaryData={summaryData}
          autoContributeEnabled={false}
          onExternalWalletAction={host.handleExternalWalletAction}
          onManageAds={onSettingsClick}
        />
        {activeView === 'tip' && <PublisherCard />}
        <NavBar
          canTip={Boolean(publisherInfo)}
          activeView={activeView}
          onActiveViewChange={setActiveView}
          onSettingsClick={onSettingsClick}
        />
      </>
    )
  }

  if (onboardingResult || !rewardsEnabled || needsCountry) {
    return renderOnboaring()
  }

  if (tosUpdateRequired) {
    return (
      <TosUpdateNotice
        onAccept={host.acceptTermsOfServiceUpdate}
        onResetRewards={host.resetRewards}
      />
    )
  }

  if (showSelfCustodyInvite) {
    return <SelfCustodyInvite />
  }

  return (
    <style.root>
      {userType !== 'unconnected' ? renderFull() : <LimitedView />}
      <PanelOverlays />
    </style.root>
  )
}
