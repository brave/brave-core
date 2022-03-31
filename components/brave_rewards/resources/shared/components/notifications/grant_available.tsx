/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { formatGrantMonth, formatGrantDaysToClaim } from '../../lib/grant_info'
import { GrantAvailableNotification, ClaimGrantAction } from './notification'
import { TokenAmount } from '../token_amount'
import { NotificationViewProps } from './notification_view'
import { GrantAvailableGraphic } from '../grant_available_graphic'

function OverlapWithCardHeader (props: { children: React.ReactChild }) {
  return (
    <div style={{ marginTop: '-20px' }}>
      {props.children}
    </div>
  )
}

export function GrantAvailable (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { grantInfo } = props.notification as GrantAvailableNotification

  function getMessages () {
    switch (grantInfo.type) {
      case 'ads':
        return {
          title: 'notificationAdGrantTitle',
          amount: 'notificationAdGrantAmount',
          button: 'notificationClaimRewards'
        }
      case 'ugp':
        return {
          title: 'notificationTokenGrantTitle',
          amount: '',
          button: 'notificationClaimTokens'
        }
    }
  }

  const messages = getMessages()
  const grantMonth = formatGrantMonth(grantInfo)
  const daysToClaim = formatGrantDaysToClaim(grantInfo)

  const action: ClaimGrantAction = {
    type: 'claim-grant',
    grantId: grantInfo.id
  }

  return (
    <div>
      <Body>
        <OverlapWithCardHeader>
          <GrantAvailableGraphic />
        </OverlapWithCardHeader>
      </Body>
      <Title style='custom'>
        {formatMessage(getString(messages.title), [grantMonth])}
      </Title>
      {
        grantInfo.amount > 0 && messages.amount &&
          <Body>
            {
              formatMessage(getString(messages.amount), [
                grantMonth,
                <strong key='amount'>
                  <TokenAmount
                    amount={grantInfo.amount}
                    minimumFractionDigits={1}
                  />
                </strong>
              ])
            }
          </Body>
      }
      <Action
        notification={props.notification}
        label={getString(messages.button)}
        action={action}
      />
      {
        daysToClaim &&
          <Body>
            {
              formatMessage(getString('notificationGrantDaysRemaining'), [
                <strong key='days'>{daysToClaim}</strong>
              ])
            }
          </Body>
      }
    </div>
  )
}
