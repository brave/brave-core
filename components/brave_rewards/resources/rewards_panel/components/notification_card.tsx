/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { HostContext } from '../lib/host_context'
import { CloseIcon } from '../../shared/components/icons/close_icon'
import { DateIcon } from './icons/date_icon'
import { MoneyBagIcon } from '../../shared/components/icons/money_bag_icon'
import { NotificationInfoIcon } from './icons/notification_info_icon'
import { NotificationErrorIcon } from './icons/notification_error_icon'

import {
  getNotificationView,
  Notification,
  NotificationActionViewProps,
  NotificationBodyProps,
  NotificationTitleProps
} from '../../shared/components/notifications'

import * as style from './notification_card.style'

const dateFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'short',
  day: 'numeric'
})

function Title (props: NotificationTitleProps) {
  function renderIcon () {
    switch (props.style) {
      case 'funding': return <MoneyBagIcon />
      case 'error': return <NotificationErrorIcon />
      default: return <NotificationInfoIcon />
    }
  }

  return (
    <style.title className={props.style || 'information'}>
      {renderIcon()}{props.text}
    </style.title>
  )
}

function Body (props: NotificationBodyProps) {
  return (
    <style.body>
      {props.children}
    </style.body>
  )
}

function Action (props: NotificationActionViewProps) {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  function onActionClick () {
    if (props.action) {
      host.handleNotificationAction(props.action)
    }
    host.dismissNotification(props.notification)
  }

  return (
    <style.action>
      <button onClick={onActionClick} data-test-id='notification-action-button'>
        {props.label || getString('ok')}
      </button>
    </style.action>
  )
}

interface Props {
  notification: Notification
}

export function NotificationCard (props: Props) {
  const host = React.useContext(HostContext)
  const View = getNotificationView(props.notification)

  function dismissNotification () {
    host.dismissNotification(props.notification)
  }

  return (
    <style.root>
      <style.header>
        <style.date>
          <DateIcon />
          {dateFormatter.format(new Date(props.notification.timeStamp))}
        </style.date>
        <style.close>
          <button onClick={dismissNotification}>
            <CloseIcon />
          </button>
        </style.close>
      </style.header>
      <style.content>
        <View
          notification={props.notification}
          Title={Title}
          Body={Body}
          Action={Action}
        />
      </style.content>
    </style.root>
  )
}
