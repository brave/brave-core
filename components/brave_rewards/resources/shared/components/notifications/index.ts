/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Notification } from './notification'
import { NotificationView } from './notification_view'

import { AddFunds } from './add_funds'
import { AutoContributeCompleted } from './auto_contribute_completed'
import { MonthlyTipCompleted } from './monthly_tip_completed'
import { MonthlyContributionFailed } from './monthly_contribution_failed'
import { GrantAvailable } from './grant_available'
import { PendingPublisherVerified } from './pending_publisher_verified'
import { PendingTipFailed } from './pending_tip_failed'
import { ExternalWalletDisconnected } from './external_wallet_disconnected'

export * from './notification'
export * from './notification_view'

export function getNotificationView (
  notification: Notification
): NotificationView {
  switch (notification.type) {
    case 'add-funds':
      return AddFunds
    case 'auto-contribute-completed':
      return AutoContributeCompleted
    case 'monthly-tip-completed':
      return MonthlyTipCompleted
    case 'monthly-contribution-failed':
      return MonthlyContributionFailed
    case 'grant-available':
      return GrantAvailable
    case 'pending-publisher-verified':
      return PendingPublisherVerified
    case 'pending-tip-failed':
      return PendingTipFailed
    case 'external-wallet-disconnected':
      return ExternalWalletDisconnected
  }
}
