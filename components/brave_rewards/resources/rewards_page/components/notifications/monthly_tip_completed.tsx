/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAppActions } from '../../lib/app_context'
import { NotificationViewProps } from './notification_view'

export function MonthlyTipCompleted(props: NotificationViewProps) {
  const { getString } = useAppActions()
  const { Title, Body, Action } = props

  return (
    <div>
      <Title style='funding'>
        {getString('notificationMonthlyTipCompletedTitle')}
      </Title>
      <Body>{getString('notificationMonthlyTipCompletedText')}</Body>
      <Action notification={props.notification} />
    </div>
  )
}
