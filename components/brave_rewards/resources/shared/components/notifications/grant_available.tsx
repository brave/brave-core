/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { GrantAvailableNotification, ClaimGrantAction } from './notification'
import { NotificationViewProps } from './notification_view'

export function GrantAvailable (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { source, grantId } = props.notification as GrantAvailableNotification

  function getDetails () {
    switch (source) {
      case 'ads':
        return {
          title: 'notificationAdGrantTitle',
          message: 'notificationAdGrantText'
        }
      case 'ugp':
        return {
          title: 'notificationTokenGrantTitle',
          message: 'notificationTokenGrantText'
        }
    }
  }

  const { title, message } = getDetails()
  const action: ClaimGrantAction = { type: 'claim-grant', grantId }

  return (
    <div>
      <Title style='funding' text={getString(title)} />
      <Body>{getString(message)}</Body>
      <Action
        notification={props.notification}
        label={getString('notificationClaim')}
        action={action}
      />
    </div>
  )
}
