/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { getExternalWalletProviderName } from '../../lib/external_wallet'
import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWalletDisconnectedNotification } from './notification'
import { NotificationViewProps } from './notification_view'

export function ExternalWalletDisconnected (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { provider } = props.notification as ExternalWalletDisconnectedNotification

  return (
    <div>
      <Title style='error'>
        {getString('notificationWalletDisconnectedTitle')}
      </Title>
      <Body>
        {
          formatMessage(getString('notificationWalletDisconnectedText'),
            [getExternalWalletProviderName(provider)])
        }
      </Body>
      <Action
        notification={props.notification}
        label={getString('notificationWalletDisconnectedAction')}
        action={{ type: 'reconnect-external-wallet' }}
      />
    </div>
  )
}
