/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { OnboardingResult, RewardsOptInModal } from '../../shared/components/onboarding'
import { AdaptiveCaptchaView } from '../../rewards_panel/components/adaptive_captcha_view'
import { GrantCaptchaModal } from './grant_captcha_modal'
import { NotificationOverlay } from './notification_overlay'
import { VBATNoticeModal } from './vbat_notice_modal'
import { shouldShowVBATNotice } from '../../shared/components/vbat_notice'
import { TabOpenerContext } from '../../shared/components/new_tab_link'

import * as urls from '../../shared/lib/rewards_urls'

export function PanelOverlays() {
  const host = React.useContext(HostContext)
  const tabOpener = React.useContext(TabOpenerContext)

  const [requestedView, setRequestedView] =
    React.useState(host.state.requestedView)
  const [rewardsEnabled, setRewardsEnabled] =
    React.useState(host.state.rewardsEnabled)
  const [declaredCountry, setDeclaredCountry] =
    React.useState(host.state.declaredCountry)
  const [availableCountries, setAvailableCountries] =
    React.useState(host.state.availableCountries)
  const [defaultCountry, setDefaultCountry] =
    React.useState(host.state.defaultCountry)
  const [options, setOptions] = React.useState(host.state.options)
  const [grantCaptchaInfo, setGrantCaptchaInfo] =
    React.useState(host.state.grantCaptchaInfo)
  const [adaptiveCaptchaInfo, setAdaptiveCaptchaInfo] =
    React.useState(host.state.adaptiveCaptchaInfo)
  const [notifications, setNotifications] =
    React.useState(host.state.notifications)
  const [userType, setUserType] = React.useState(host.state.userType)
  const [notificationsHidden, setNotificationsHidden] = React.useState(false)
  const [hideVBATNotice, setHideVBATNotice] = React.useState(false)
  const [onboardingResult, setOnboardingResult] =
    React.useState<OnboardingResult | null>(null)

  const needsCountry = rewardsEnabled && !declaredCountry

  useHostListener(host, (state) => {
    setRequestedView(state.requestedView)
    setRewardsEnabled(state.rewardsEnabled)
    setDeclaredCountry(state.declaredCountry)
    setAvailableCountries(state.availableCountries)
    setDefaultCountry(state.defaultCountry)
    setOptions(state.options)
    setGrantCaptchaInfo(state.grantCaptchaInfo)
    setNotifications(state.notifications)
    setAdaptiveCaptchaInfo(state.adaptiveCaptchaInfo)
    setUserType(state.userType)
  })

  if (onboardingResult || !rewardsEnabled || needsCountry) {
    const onHideResult = () => {
      if (onboardingResult === 'success') {
        setTimeout(() => {
          tabOpener.openTab(urls.rewardsTourURL)
        }, 2000)
      }
      setOnboardingResult(null)
    }

    const onEnable = (country: string) => {
      host.enableRewards(country).then((result) => {
        setOnboardingResult(result)
      })
    }

    return (
      <RewardsOptInModal
        availableCountries={availableCountries}
        defaultCountry={defaultCountry}
        initialView={needsCountry ?
          'declare-country' : requestedView === 'declare-country' ?
            'skip-to-declare-country' : 'default'}
        result={onboardingResult}
        onEnable={onEnable}
        onHideResult={onHideResult}
      />
    )
  }

  if (adaptiveCaptchaInfo) {
    const onContactSupport = () => {
      host.clearAdaptiveCaptcha()
      host.openAdaptiveCaptchaSupport()
    }

    const onClose = () => {
      host.clearAdaptiveCaptcha()
    }

    return (
      <AdaptiveCaptchaView
        adaptiveCaptchaInfo={adaptiveCaptchaInfo}
        onClose={onClose}
        onCaptchaResult={host.handleAdaptiveCaptchaResult}
        onContactSupport={onContactSupport}
      />
    )
  }

  if (grantCaptchaInfo) {
    return (
      <GrantCaptchaModal
        grantCaptchaInfo={grantCaptchaInfo}
        onSolve={host.solveGrantCaptcha}
        onClose={host.clearGrantCaptcha}
      />
    )
  }

  if (notifications.length > 0 && !notificationsHidden) {
    const onClose = () => { setNotificationsHidden(true) }
    return (
      <NotificationOverlay notifications={notifications} onClose={onClose} />
    )
  }

  if (!hideVBATNotice && shouldShowVBATNotice(userType, options.vbatDeadline)) {
    const onClose = () => { setHideVBATNotice(true) }
    const onConnect = () => { host.handleExternalWalletAction('verify') }
    return (
      <VBATNoticeModal
        onClose={onClose}
        onConnectAccount={onConnect}
      />
    )
  }

  return null
}
