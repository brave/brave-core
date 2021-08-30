/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { TokenAmount } from '../../components/token_amount'
import { AutoContributeCompletedNotification } from './notification'

import { NotificationViewProps } from './notification_view'

export function AutoContributeCompleted (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { amount } = props.notification as AutoContributeCompletedNotification

  return (
    <div>
      <Title
        style='funding'
        text={getString('notificationAutoContributeCompletedTitle')}
      />
      <Body>
        {
          formatMessage(getString('notificationAutoContributeCompletedText'), [
            <TokenAmount key='amount' amount={amount} />
          ])
        }
      </Body>
      <Action notification={props.notification} />
    </div>
  )
}
