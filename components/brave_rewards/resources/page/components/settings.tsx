/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { LocaleContext } from '../../shared/lib/locale_context'
import { LayoutContext } from '../lib/layout_context'

import PageWallet from './pageWallet'
import AdsBox from './adsBox'
import ContributeBox from './contributeBox'
import TipBox from './tipsBox'
import MonthlyTipsBox from './monthlyTipsBox'
import { SettingsOptInForm, RewardsTourModal } from '../../shared/components/onboarding'
import { ProviderRedirectModal } from './provider_redirect_modal'
import { GrantList } from './grant_list'
import { SidebarPromotionPanel } from './sidebar_promotion_panel'
import { BatIcon } from '../../shared/components/icons/bat_icon'

import * as style from './settings.style'

export function Settings () {
  const layoutKind = React.useContext(LayoutContext)
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()
  const rewardsData = useRewardsData((data) => data)

  const [showRewardsTour, setShowRewardsTour] = React.useState(false)
  const [firstTimeSetup, setFirstTimeSetup] = React.useState(false)

  const handleURL = () => {
    // Used by Android to disconnect the user's external wallet.
    if (location.hash === '#disconnect-wallet') {
      actions.disconnectWallet()
      return true
    }

    const { pathname } = location

    // Used to enable Rewards directly from the Welcome UI.
    if (pathname === '/enable') {
      actions.saveOnboardingResult('opted-in')
      setShowRewardsTour(true)
      setFirstTimeSetup(true)
      return true
    }

    // Allow the browser to handle any URL that has 2 or more path components.
    if (pathname.split('/').length > 2) {
      actions.processRewardsPageUrl(pathname, location.search)
      return true
    }

    return false
  }

  React.useEffect(() => {
    const date = new Date()
    actions.getBalanceReport(date.getMonth() + 1, date.getFullYear())
    actions.getTipTable()
    actions.getPendingContributions()
    actions.getStatement()
    actions.getAdsData()
    actions.getExcludedSites()
    actions.getCountryCode()
    actions.getRewardsParameters()
    actions.getContributionAmount()
    actions.getAutoContributeProperties()
    actions.getBalance()
    actions.fetchPromotions()
    actions.getOnboardingStatus()
    actions.getEnabledInlineTippingPlatforms()

    if (handleURL()) {
      history.replaceState({}, '', '/')
    }
  }, [])

  React.useEffect(() => {
    const id = setInterval(() => { actions.getBalance() }, 60000)
    return () => { clearInterval(id) }
  }, [])

  React.useEffect(() => {
    actions.getContributeList()
    actions.getReconcileStamp()
  }, [rewardsData.enabledContribute])

  React.useEffect(() => {
    if (!rewardsData.externalWallet) {
      actions.getExternalWallet()
    }
  }, [rewardsData.externalWallet])

  const onTakeTour = () => { setShowRewardsTour(true) }

  const renderRewardsTour = () => {
    if (!showRewardsTour) {
      return null
    }

    const {
      adsData,
      contributionMonthly,
      externalWallet,
      parameters
    } = rewardsData

    const onDone = () => {
      setShowRewardsTour(false)
      setFirstTimeSetup(false)
    }

    const onAdsPerHourChanged = (adsPerHour: number) => {
      actions.onAdsSettingSave('adsPerHour', adsPerHour)
    }

    const onAcAmountChanged = (amount: number) => {
      actions.onSettingSave('contributionMonthly', amount)
    }

    const onVerifyClick = () => {
      if (externalWallet && externalWallet.loginUrl) {
        window.open(externalWallet.loginUrl, '_self')
      }
    }

    return (
      <RewardsTourModal
        layout={layoutKind}
        firstTimeSetup={firstTimeSetup}
        adsPerHour={adsData.adsPerHour}
        autoContributeAmount={contributionMonthly}
        autoContributeAmountOptions={parameters.autoContributeChoices}
        externalWalletProvider={externalWallet ? externalWallet.type : ''}
        onAdsPerHourChanged={onAdsPerHourChanged}
        onAutoContributeAmountChanged={onAcAmountChanged}
        onVerifyWalletClick={onVerifyClick}
        onDone={onDone}
        onClose={onDone}
      />
    )
  }

  const renderOnboarding = () => {
    const onEnable = () => {
      actions.saveOnboardingResult('opted-in')
      setShowRewardsTour(true)
      setFirstTimeSetup(true)
    }

    return (
      <style.onboarding>
        <SettingsOptInForm onTakeTour={onTakeTour} onEnable={onEnable} />
      </style.onboarding>
    )
  }

  const renderContent = () => {
    if (rewardsData.showOnboarding) {
      return renderOnboarding()
    }

    return (
      <style.content>
        <style.main>
          <style.title>
            <BatIcon />
            {getString('braveRewards')}
          </style.title>
          <style.settingGroup>
            <AdsBox layout={layoutKind} />
          </style.settingGroup>
          <style.settingGroup data-test-id='auto-contribute-settings'>
            <ContributeBox />
          </style.settingGroup>
          <style.settingGroup>
            <TipBox />
          </style.settingGroup>
          <style.settingGroup>
            <MonthlyTipsBox />
          </style.settingGroup>
        </style.main>
        <style.sidebar>
          <style.grants>
            <GrantList />
          </style.grants>
          <style.rewardsCard>
            <PageWallet layout={layoutKind} />
          </style.rewardsCard>
          <style.promotions>
            <SidebarPromotionPanel onTakeRewardsTour={onTakeTour} />
          </style.promotions>
        </style.sidebar>
      </style.content>
    )
  }

  return (
    <style.root className={`layout-${layoutKind}`}>
      <ProviderRedirectModal />
      {renderRewardsTour()}
      {renderContent()}
    </style.root>
  )
}
