/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWalletProvider } from '../../lib/external_wallet'
import { GrantInfo } from '../../lib/grant_info'

export type NotificationType =
  'auto-contribute-completed' |
  'monthly-tip-completed' |
  'monthly-contribution-failed' |
  'grant-available' |
  'pending-publisher-verified' |
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

export interface GrantAvailableNotification extends Notification {
  type: 'grant-available'
  grantInfo: GrantInfo
}

export interface PendingPublisherVerifiedNotification extends Notification {
  type: 'pending-publisher-verified'
  publisherName: string
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
  'claim-grant' |
  'reconnect-external-wallet'

export interface NotificationAction {
  type: NotificationActionType
}

export interface OpenLinkAction extends NotificationAction {
  type: 'open-link'
  url: string
}

export interface ClaimGrantAction extends NotificationAction {
  type: 'claim-grant'
  grantId: string
}
