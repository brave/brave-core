/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ExternalWalletProvider } from '../../lib/external_wallet'

export type NotificationType =
  'add-funds' |
  'auto-contribute-completed' |
  'monthly-tip-completed' |
  'monthly-contribution-failed' |
  'grant-available' |
  'backup-wallet' |
  'pending-publisher-verified' |
  'pending-tip-failed' |
  'external-wallet-verified' |
  'external-wallet-disconnected' |
  'external-wallet-linking-failed'

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
  reason: 'insufficient-funds' | 'unknown'
}

export interface GrantAvailableNotification extends Notification {
  type: 'grant-available'
  source: 'ads' | 'ugp'
  grantId: string
}

export interface PendingPublisherVerifiedNotification extends Notification {
  type: 'pending-publisher-verified'
  publisherName: string
}

export interface PendingTipFailedNotification extends Notification {
  type: 'pending-tip-failed'
  reason: 'insufficient-funds'
}

export interface ExternalWalletVerifiedNotification extends Notification {
  type: 'external-wallet-verified'
  provider: ExternalWalletProvider
}

export interface ExternalWalletDisconnectedNotification extends Notification {
  type: 'external-wallet-disconnected'
  provider: ExternalWalletProvider
}

export interface ExternalWalletLinkingFailedNotification extends Notification {
  type: 'external-wallet-linking-failed'
  provider: ExternalWalletProvider
  reason:
    'device-limit-reached' |
    'mismatched-provider-accounts' |
    'uphold-bat-not-supported' |
    'uphold-user-blocked' |
    'uphold-user-pending' |
    'uphold-user-restricted'
}

export type NotificationActionType =
  'open-link' |
  'claim-grant' |
  'backup-wallet' |
  'add-funds' |
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
