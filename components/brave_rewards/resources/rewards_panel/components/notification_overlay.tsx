/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Notification } from '../../shared/components/notifications'
import { NotificationCard } from './notification_card'

import * as style from './notification_overlay.style'

interface Props {
  notifications: Notification[]
  onClose: () => void
}

export function NotificationOverlay (props: Props) {
  function onBackgroundClick (evt: React.UIEvent) {
    if (evt.target === evt.currentTarget) {
      props.onClose()
    }
  }

  if (props.notifications.length === 0) {
    return null
  }

  const sortedNotifications = Array.from(props.notifications).sort((a, b) => {
    return b.timeStamp - a.timeStamp
  })

  return (
    <style.root onClick={onBackgroundClick}>
      <style.card>
        <NotificationCard notification={sortedNotifications[0]} />
        {props.notifications.length > 1 && <style.peek />}
      </style.card>
    </style.root>
  )
}
