/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWalletProvider } from '../../lib/external_wallet'

export type NotificationType =
  'auto-contribute-completed' |
  'monthly-tip-completed' |
  'monthly-contribution-failed' |
  'external-wallet-disconnected' |
  'uphold-bat-not-allowed' |
  'uphold-insufficient-capabilities'

export interface Notification {
  type: NotificationType
  id: string
  timeStamp: number
}

export interface AutoContributeCompletedNotification extends Notification {
  type: 'auto-contribute-completed'
  amount: number
}

export interface MonthlyContributionFailedNotification extends Notification {
  type: 'monthly-contribution-failed'
  reason: 'unknown'
}

export interface ExternalWalletDisconnectedNotification extends Notification {
  type: 'external-wallet-disconnected'
  provider: ExternalWalletProvider
}

export interface UpholdBATNotAllowedNotification extends Notification {
  type: 'uphold-bat-not-allowed'
  provider: ExternalWalletProvider
}

export interface UpholdInsufficientCapabilitiesNotification extends Notification {
  type: 'uphold-insufficient-capabilities'
  provider: ExternalWalletProvider
}

export type NotificationActionType =
  'open-link' |
  'reconnect-external-wallet'

export interface NotificationAction {
  type: NotificationActionType
}

export interface OpenLinkAction extends NotificationAction {
  type: 'open-link'
  url: string
}
