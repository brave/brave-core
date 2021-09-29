/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { ExternalWalletLinkingFailedNotification, OpenLinkAction } from './notification'
import { NotificationViewProps } from './notification_view'
import { getExternalWalletProviderName } from '../../lib/external_wallet'

export function ExternalWalletLinkingFailed (props: NotificationViewProps) {
  const { getString } = React.useContext(LocaleContext)
  const { Title, Body, Action } = props
  const { reason, provider } =
    props.notification as ExternalWalletLinkingFailedNotification

  function getDetails () {
    switch (reason) {
      case 'device-limit-reached':
        return {
          message: getString('notificationDeviceLimitReachedText'),
          url: 'https://support.brave.com/hc/en-us/articles/360056508071'
        }
      case 'mismatched-provider-accounts':
        return {
          message: formatMessage(
            getString('notificationMismatchedProviderAccountsText'), [
              getExternalWalletProviderName(provider)
            ]),
          url: 'https://support.brave.com/hc/en-us/articles/360034841711-What-is-a-verified-wallet-'
        }
      case 'uphold-bat-not-supported':
        return {
          message: getString('notificationUpholdBatNotSupportedText'),
          url: 'https://support.uphold.com/hc/en-us/articles/360033020351-Brave-BAT-and-US-availability '
        }
      case 'uphold-user-blocked':
        return {
          message: getString('notificationUpholdUserBlockedText'),
          url: 'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'
        }
      case 'uphold-user-pending':
        return {
          message: getString('notificationUpholdUserPendingText'),
          url: 'https://support.uphold.com/hc/en-us/articles/206695986-How-do-I-sign-up-for-Uphold-Web-'
        }
      case 'uphold-user-restricted':
        return {
          message: getString('notificationUpholdUserRestrictedText'),
          url: 'https://support.uphold.com/hc/en-us/articles/360045765351-Why-we-block-or-restrict-accounts-and-how-to-reduce-the-risk'
        }
    }
  }

  const { message, url } = getDetails()
  const action: OpenLinkAction = { type: 'open-link', url }

  return (
    <div>
      <Title
        style='error'
        text={getString('notificationWalletLinkingFailedTitle')}
      />
      <Body>{message}</Body>
      <Action
        notification={props.notification}
        label={getString('notificationLearnMore')}
        action={action}
      />
    </div>
  )
}
