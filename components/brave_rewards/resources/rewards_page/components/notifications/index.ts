/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Notification } from '../../lib/app_state'
import { NotificationView } from './notification_view'

import { MonthlyTipCompleted } from './monthly_tip_completed'
import { ExternalWalletDisconnected } from './external_wallet_disconnected'

export * from './notification_view'

export function getNotificationView(
  notification: Notification,
): NotificationView {
  switch (notification.type) {
    case 'monthly-tip-completed':
      return MonthlyTipCompleted
    case 'external-wallet-disconnected':
      return ExternalWalletDisconnected
  }
}
