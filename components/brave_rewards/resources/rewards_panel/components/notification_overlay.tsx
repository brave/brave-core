/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { HostContext, useHostListener } from '../lib/host_context'
import { NotificationCard } from './notification_card'

import * as styles from './notification_overlay.style'

export function NotificationOverlay () {
  const host = React.useContext(HostContext)

  const [notifications, setNotifications] =
    React.useState(host.state.notifications)
  const [notificationsLastViewed, setNotificationsLastViewed ] =
    React.useState(host.state.notificationsLastViewed)

  const [slideIn, setSlideIn] = React.useState(true)

  useHostListener(host, (state) => {
    setNotifications(state.notifications)
    setNotificationsLastViewed(state.notificationsLastViewed)
  })

  const onRootMounted = React.useCallback((elem: HTMLElement | null) => {
    if (elem) {
      // When attaching the overlay to the document, trigger slide-in animation.
      setTimeout(() => setSlideIn(false), 0)
    } else {
      // When removing the overlay from the document, reset the last viewed
      // notification date and set the "slide in" animation to run on next
      // render.
      host.setNotificationsViewed()
      setSlideIn(true)
    }
  }, [])

  const activeNotifications = Array.from(notifications)
    .sort((a, b) => b.timeStamp - a.timeStamp)
    .filter(n => n.timeStamp > notificationsLastViewed)

  if (activeNotifications.length === 0) {
    return null
  }

  function onBackgroundClick (evt: React.UIEvent) {
    if (evt.target === evt.currentTarget) {
      host.setNotificationsViewed()
    }
  }

  return (
    <styles.root ref={onRootMounted} onClick={onBackgroundClick}>
      <styles.card className={slideIn ? 'offstage' : ''}>
        <NotificationCard notification={activeNotifications[0]} />
        {activeNotifications.length > 1 && <styles.peek />}
      </styles.card>
    </styles.root>
  )
}
