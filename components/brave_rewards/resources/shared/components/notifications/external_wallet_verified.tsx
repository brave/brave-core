/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWalletVerifiedNotification } from './notification'
import { getExternalWalletProviderName } from '../../lib/external_wallet'

import { NotificationViewProps } from './notification_view'

export function ExternalWalletVerified (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { provider } = props.notification as ExternalWalletVerifiedNotification

  return (
    <div>
      <Title
        style='information'
        text={getString('notificationWalletVerifiedTitle')}
      />
      <Body>
        {
          formatMessage(getString('notificationWalletVerifiedText'), [
            getExternalWalletProviderName(provider)
          ])
        }
      </Body>
      <Action notification={props.notification} />
    </div>
  )
}
