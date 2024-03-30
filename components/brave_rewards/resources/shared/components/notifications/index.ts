/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Notification } from './notification'
import { NotificationView } from './notification_view'

import { AutoContributeCompleted } from './auto_contribute_completed'
import { MonthlyTipCompleted } from './monthly_tip_completed'
import { MonthlyContributionFailed } from './monthly_contribution_failed'
import { ExternalWalletDisconnected } from './external_wallet_disconnected'
import { UpholdBATNotAllowed } from './uphold_bat_not_allowed'
import { UpholdInsufficientCapabilities } from './uphold_insufficient_capabilities'

export * from './notification'
export * from './notification_view'

export function getNotificationView (
  notification: Notification
): NotificationView {
  switch (notification.type) {
    case 'auto-contribute-completed':
      return AutoContributeCompleted
    case 'monthly-tip-completed':
      return MonthlyTipCompleted
    case 'monthly-contribution-failed':
      return MonthlyContributionFailed
    case 'external-wallet-disconnected':
      return ExternalWalletDisconnected
    case 'uphold-bat-not-allowed':
      return UpholdBATNotAllowed
    case 'uphold-insufficient-capabilities':
      return UpholdInsufficientCapabilities
  }
}
