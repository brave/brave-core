/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { PendingTipFailedNotification } from './notification'
import { NotificationViewProps } from './notification_view'

export function PendingTipFailed (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { reason } = props.notification as PendingTipFailedNotification

  switch (reason) {
    case 'insufficient-funds':
      return (
        <div>
          <Title
            style='error'
            text={getString('notificationPendingTipFailedTitle')}
          />
          <Body>
            {getString('notificationPendingTipFailedText')}
          </Body>
          <Action
            notification={props.notification}
            label={getString('notificationAddFunds')}
            action={{ type: 'add-funds' }}
          />
        </div>
      )
  }
}
