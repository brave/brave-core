/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../../shared/lib/locale_context'
import { PendingPublisherVerifiedNotification } from './notification'
import { NotificationViewProps } from './notification_view'

export function PendingPublisherVerified (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { publisherName } =
    props.notification as PendingPublisherVerifiedNotification

  return (
    <div>
      <Title text={getString('notificationPublisherVerifiedTitle')} />
      <Body>
        {
          formatMessage(getString('notificationPublisherVerifiedText'), [
            <strong key='name'>{publisherName}</strong>
          ])
        }
      </Body>
      <Action notification={props.notification} />
    </div>
  )
}
