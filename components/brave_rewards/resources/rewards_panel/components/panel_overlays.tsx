/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { BraveTalkOptInForm, RewardsOptInModal, RewardsTourModal } from '../../shared/components/onboarding'
import { AdaptiveCaptchaView } from '../../rewards_panel/components/adaptive_captcha_view'
import { GrantCaptchaModal } from './grant_captcha_modal'
import { NotificationOverlay } from './notification_overlay'

// Attaches a CSS class to the document body containing the name of the overlay.
// This allows root-level style rules to expand the height of the panel if
// necessary, based on the currently displayed overlay.
// TODO(zenparsing): Create a mechanism that will automatically resize the
// document based on the height of the overlay. See |ResizeObserver|.
function NamedOverlay (props: { name: string, children: React.ReactNode }) {
  const onMountUnmount = (elem: HTMLElement | null) => {
    const className = `panel-overlay-${props.name}`
    if (elem) {
      document.body.classList.add(className)
    } else {
      document.body.classList.remove(className)
    }
  }

  return (
    <div ref={onMountUnmount}>
      {props.children}
    </div>
  )
}

export function PanelOverlays () {
  const host = React.useContext(HostContext)

  const [requestedView, setRequestedView] =
    React.useState(host.state.requestedView)
  const [rewardsEnabled, setRewardsEnabled] =
    React.useState(host.state.rewardsEnabled)
  const [settings, setSettings] = React.useState(host.state.settings)
  const [options, setOptions] = React.useState(host.state.options)
  const [externalWalletProviders, setExternalWalletProviders] =
    React.useState(host.state.externalWalletProviders)
  const [grantCaptchaInfo, setGrantCaptchaInfo] =
    React.useState(host.state.grantCaptchaInfo)
  const [adaptiveCaptchaInfo, setAdaptiveCaptchaInfo] =
    React.useState(host.state.adaptiveCaptchaInfo)
  const [notifications, setNotifications] =
    React.useState(host.state.notifications)

  const [showTour, setShowTour] = React.useState(false)
  const [showTalkOptIn, setShowTalkOptIn] = React.useState(false)
  const [notificationsHidden, setNotificationsHidden] = React.useState(false)

  useHostListener(host, (state) => {
    setRequestedView(state.requestedView)
    setRewardsEnabled(state.rewardsEnabled)
    setSettings(state.settings)
    setOptions(state.options)
    setExternalWalletProviders(state.externalWalletProviders)
    setGrantCaptchaInfo(state.grantCaptchaInfo)
    setNotifications(state.notifications)
    setAdaptiveCaptchaInfo(state.adaptiveCaptchaInfo)
  })

  React.useEffect(() => {
    switch (requestedView) {
      case 'rewards-tour':
        setShowTour(true)
        break
      case 'brave-talk-opt-in':
        setShowTalkOptIn(true)
        break
    }
  }, [requestedView])

  function toggleTour () {
    setShowTour(!showTour)
  }

  function onEnable () {
    host.enableRewards()
    setShowTour(true)
  }

  if (showTour) {
    const onVerifyWalletClick = () => {
      host.handleExternalWalletAction('verify')
    }

    return (
      <NamedOverlay name='rewards-tour'>
        <RewardsTourModal
          firstTimeSetup={rewardsEnabled}
          adsPerHour={settings.adsPerHour}
          externalWalletProvider={externalWalletProviders[0]}
          autoContributeAmount={settings.autoContributeAmount}
          autoContributeAmountOptions={options.autoContributeAmounts}
          onAdsPerHourChanged={host.setAdsPerHour}
          onAutoContributeAmountChanged={host.setAutoContributeAmount}
          onVerifyWalletClick={onVerifyWalletClick}
          onDone={toggleTour}
          onClose={toggleTour}
        />
      </NamedOverlay>
    )
  }

  if (showTalkOptIn) {
    return (
      <NamedOverlay name='brave-talk-opt-in'>
        <BraveTalkOptInForm
          showRewardsOnboarding={!rewardsEnabled}
          onEnable={host.enableRewards}
          onTakeTour={toggleTour}
        />
      </NamedOverlay>
    )
  }

  if (!rewardsEnabled) {
    return (
      <NamedOverlay name='opt-in'>
        <RewardsOptInModal onEnable={onEnable} onTakeTour={toggleTour} />
      </NamedOverlay>
    )
  }

  if (adaptiveCaptchaInfo) {
    const onClose = () => {
      host.clearAdaptiveCaptcha()
    }

    return (
      <NamedOverlay name='adaptive-captcha'>
        <AdaptiveCaptchaView
          adaptiveCaptchaInfo={adaptiveCaptchaInfo}
          onClose={onClose}
          onCaptchaResult={host.handleAdaptiveCaptchaResult}
        />
      </NamedOverlay>
    )
  }

  if (grantCaptchaInfo) {
    return (
      <NamedOverlay name='grant-captcha'>
        <GrantCaptchaModal
          grantCaptchaInfo={grantCaptchaInfo}
          onSolve={host.solveGrantCaptcha}
          onClose={host.clearGrantCaptcha}
        />
      </NamedOverlay>
    )
  }

  if (notifications.length > 0 && !notificationsHidden) {
    const onClose = () => { setNotificationsHidden(true) }
    return (
      <NamedOverlay name='notifications'>
        <NotificationOverlay notifications={notifications} onClose={onClose} />
      </NamedOverlay>
    )
  }

  return null
}
