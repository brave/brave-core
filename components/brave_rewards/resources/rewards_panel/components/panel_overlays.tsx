/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { AdaptiveCaptchaView } from '../../rewards_panel/components/adaptive_captcha_view'
import { NotificationOverlay } from './notification_overlay'

export function PanelOverlays() {
  const host = React.useContext(HostContext)

  const [adaptiveCaptchaInfo, setAdaptiveCaptchaInfo] =
    React.useState(host.state.adaptiveCaptchaInfo)
  const [notifications, setNotifications] =
    React.useState(host.state.notifications)
  const [notificationsHidden, setNotificationsHidden] = React.useState(false)

  useHostListener(host, (state) => {
    setNotifications(state.notifications)
    setAdaptiveCaptchaInfo(state.adaptiveCaptchaInfo)
  })

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

  if (notifications.length > 0 && !notificationsHidden) {
    const onClose = () => { setNotificationsHidden(true) }
    return (
      <NotificationOverlay notifications={notifications} onClose={onClose} />
    )
  }

  return null
}
