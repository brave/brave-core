/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { RewardsOptInModal, RewardsTourModal } from '../../shared/components/onboarding'
import { GrantCaptchaModal } from './grant_captcha_modal'
import { NotificationOverlay } from './notification_overlay'

// Attaches a CSS class to the document body containing the name of the overlay.
// This allows root-level style rules to expand the height of the panel if
// necessary, based on the currently displayed overlay.
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

  const [rewardsEnabled, setRewardsEnabled] =
    React.useState(host.state.rewardsEnabled)
  const [settings, setSettings] = React.useState(host.state.settings)
  const [options, setOptions] = React.useState(host.state.options)
  const [externalWalletProviders, setExternalWalletProviders] =
    React.useState(host.state.externalWalletProviders)
  const [grantCaptchaInfo, setGrantCaptchaInfo] =
    React.useState(host.state.grantCaptchaInfo)
  const [notifications, setNotifications] =
    React.useState(host.state.notifications)

  const [showTour, setShowTour] = React.useState(false)
  const [notificationsHidden, setNotificationsHidden] = React.useState(false)

  useHostListener(host, (state) => {
    setRewardsEnabled(state.rewardsEnabled)
    setSettings(state.settings)
    setOptions(state.options)
    setExternalWalletProviders(state.externalWalletProviders)
    setGrantCaptchaInfo(state.grantCaptchaInfo)
    setNotifications(state.notifications)
  })

  React.useEffect(() => {
    // After the component is mounted, check for a "#tour" URL and display the
    // rewards tour if found.
    if ((/^#?tour$/i).test(location.hash)) {
      setShowTour(true)
      location.hash = ''
    }
  }, [])

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

  if (!rewardsEnabled) {
    return (
      <NamedOverlay name='opt-in'>
        <RewardsOptInModal onEnable={onEnable} onTakeTour={toggleTour} />
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
