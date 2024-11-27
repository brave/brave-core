/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useActions, useRewardsData } from '../lib/redux_hooks'
import { PlatformContext } from '../lib/platform_context'
import { LocaleContext } from '../../shared/lib/locale_context'
import { LayoutContext } from '../lib/layout_context'
import { isSelfCustodyProvider } from '../../shared/lib/external_wallet'

import PageWallet from './pageWallet'

import { AdsPanel } from './ads_panel'
import { TipsPanel } from './tips_panel'
import { MonthlyTipsPanel } from './monthly_tips_panel'
import { SettingsOptInForm } from '../../shared/components/onboarding'
import { ProviderRedirectModal } from './provider_redirect_modal'
import { SidebarPromotionPanel } from './sidebar_promotion_panel'
import { UnsupportedRegionNotice } from './unsupported_region_notice'
import { BatIcon } from '../../shared/components/icons/bat_icon'
import { SettingsIcon } from '../../shared/components/icons/settings_icon'

import * as style from './settings.style'
import { Action } from 'redux'

export function Settings () {
  const { isAndroid } = React.useContext(PlatformContext)
  const layoutKind = React.useContext(LayoutContext)
  const { getString } = React.useContext(LocaleContext)
  const actions = useActions()
  const rewardsData = useRewardsData((data) => data)

  const scrollToDeeplinkId = (deeplinkId: string) => {
    if (deeplinkId) {
      if (deeplinkId === 'top') {
        window.scrollTo(0, 0)
        return true
      }

      const element = document.querySelector(
        `[data-deeplink-id="${deeplinkId}"`)
      if (element) {
        element.scrollIntoView()
        return true
      }
    }
    return false
  }

  const handleURLActions = () => {
    const { pathname } = location

    // Used to enable Rewards directly from the Welcome UI.
    if (pathname === '/enable') {
      actions.enableRewards()
      return true
    }

    if (pathname.includes('authorization')) {
      actions.connectExternalWallet(pathname, location.search)
      return true
    }

    return false
  }

  class HashHandler {
    hash: string;
    deeplinkId: string;
    action: (() => Action<any>) | null;
    actionIsForSettings: boolean;

    constructor(
      hash: string,
      action?: () => Action<any>,
      actionIsForSettings?: boolean,
      deeplinkId?: string) {
      this.hash = hash
      this.action = action ?? null
      this.actionIsForSettings = actionIsForSettings ?? false
      this.deeplinkId = deeplinkId ?? this.hash
    }
  }

  const hashHandlers: HashHandler[] = [
    new HashHandler('ads', actions.onAdsSettingsOpen, true),
    new HashHandler(
      'auto-contribute', actions.onAutoContributeSettingsOpen, true),
    new HashHandler('monthly-contributions'),
    new HashHandler('ads-history', actions.onModalAdsHistoryOpen, false, 'ads'),
    new HashHandler('reset', actions.onModalResetOpen, false, 'top')
  ]

  const handleURLDeepLinks = () => {
    if (!location.hash) {
      return
    }
    hashHandlers.every((handler) => {
      if (location.hash === '#' + handler.hash) {
        if (scrollToDeeplinkId(handler.deeplinkId)) {
          if (handler.action) {
            if (!handler.actionIsForSettings) {
              handler.action()
            } else if (location.search === '?settings') {
              handler.action()
            }
          }
          clearURLPath()
        }
        return false
      }
      return true
    })
  }

  const clearURLPath = () => {
    history.replaceState({}, '', '/')
  }

  React.useEffect(() => {
    actions.getUserType()
    actions.isTermsOfServiceUpdateRequired()
    actions.getIsUnsupportedRegion()
    const date = new Date()
    actions.getBalanceReport(date.getMonth() + 1, date.getFullYear())
    actions.getTipTable()
    actions.getStatement()
    actions.getAdsData()
    actions.getExcludedSites()
    actions.getCountryCode()
    actions.getRewardsParameters()
    actions.getContributionAmount()
    actions.getIsAutoContributeSupported()
    actions.getAutoContributeProperties()
    actions.getBalance()
    actions.getExternalWallet()
    actions.getOnboardingStatus()

    if (handleURLActions()) {
      clearURLPath()
    } else {
      handleURLDeepLinks()
    }
  }, [rewardsData.initializing])

  React.useEffect(() => {
    const id = setInterval(() => { actions.getBalance() }, 180000)
    return () => { clearInterval(id) }
  }, [rewardsData.initializing])

  React.useEffect(() => {
    actions.getContributeList()
    actions.getReconcileStamp()
  }, [rewardsData.enabledContribute])

  const shouldShowTips = () => {
    if (rewardsData.userType === 'unconnected') {
      return false
    }
    const { externalWallet } = rewardsData
    if (externalWallet && isSelfCustodyProvider(externalWallet.type)) {
      return false
    }
    return true
  }

  const onManageClick = () => { actions.onModalResetOpen() }

  function renderUnsupportedRegionNotice () {
    return (
      <div>
        <style.unsupportedRegionNoticeTitle>
          <style.header>
            <style.title>
              <BatIcon />{getString('braveRewards')}
            </style.title>
          </style.header>
        </style.unsupportedRegionNoticeTitle>
        <style.unsupportedRegionNotice>
          <UnsupportedRegionNotice />
        </style.unsupportedRegionNotice>
      </div>
    )
  }

  function renderOnboarding () {
    const onEnable = () => {
      actions.enableRewards()
    }

    return (
      <style.onboarding>
        <SettingsOptInForm onEnable={isAndroid ? undefined : onEnable} />
      </style.onboarding>
    )
  }

  function renderContent () {
    // Do not display content until the user's onboarding status has been
    // determined.
    if (rewardsData.showOnboarding === null) {
      return null
    }

    if (rewardsData.isUnsupportedRegion) {
      return renderUnsupportedRegionNotice()
    }

    if (rewardsData.showOnboarding) {
      return renderOnboarding()
    }

    return (
      <style.content>
        <style.main>
          <style.header>
            <style.title>
              <BatIcon />{getString('braveRewards')}
            </style.title>
            <style.manageAction>
              <button
                onClick={onManageClick}
                data-test-id='manage-wallet-button'
              >
                <SettingsIcon />{getString('reset')}
              </button>
            </style.manageAction>
          </style.header>
          <style.settingGroup>
            <AdsPanel />
          </style.settingGroup>
          {
            shouldShowTips() &&
              <>
                <style.settingGroup>
                  <TipsPanel />
                </style.settingGroup>
                <style.settingGroup>
                  <MonthlyTipsPanel />
                </style.settingGroup>
              </>
          }
        </style.main>
        <style.sidebar>
          <PageWallet />
          <SidebarPromotionPanel />
        </style.sidebar>
      </style.content>
    )
  }

  return (
    <style.root className={`layout-${layoutKind}`}>
      <ProviderRedirectModal />
      {renderContent()}
    </style.root>
  )
}
