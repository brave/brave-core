/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { NotificationViewProps } from './notification_view'

export function UpholdInsufficientCapabilities (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props

  return (
    <div>
      <Title style='error'>
        {getString('notificationUpholdInsufficientCapabilitiesTitle')}
      </Title>
      <Body>{getString('notificationUpholdInsufficientCapabilitiesText')}</Body>
      <Action notification={props.notification}/>
    </div>
  )
}
