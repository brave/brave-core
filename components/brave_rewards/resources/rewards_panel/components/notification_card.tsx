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

import * as styles from './notification_card.style'

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
    <styles.title className={props.style || 'information'}>
      {renderIcon()}{props.text}
    </styles.title>
  )
}

function Body (props: NotificationBodyProps) {
  return (
    <styles.body>
      {props.children}
    </styles.body>
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
    <styles.action>
      <button onClick={onActionClick}>
        {props.label || getString('notificationOK')}
      </button>
    </styles.action>
  )
}

interface Props {
  notification: Notification
}

export function NotificationCard (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const host = React.useContext(HostContext)
  const View = getNotificationView(props.notification)

  function dismissNotification () {
    host.dismissNotification(props.notification)
  }

  return (
    <styles.root>
      <styles.header>
        <styles.date>
          <DateIcon />
          {dateFormatter.format(new Date(props.notification.timeStamp))}
        </styles.date>
        <styles.close>
          <button onClick={dismissNotification}>
            <CloseIcon />
          </button>
        </styles.close>
      </styles.header>
      <styles.content>
        <View
          notification={props.notification}
          Title={Title}
          Body={Body}
          Action={Action}
        />
        <styles.dismiss>
          <button onClick={dismissNotification}>
            {getString('notificationDismiss')}
          </button>
        </styles.dismiss>
      </styles.content>
    </styles.root>
  )
}
