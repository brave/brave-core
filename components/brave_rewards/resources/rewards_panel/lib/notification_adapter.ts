/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  ExternalWalletProvider,
  externalWalletProviderFromString
} from '../../shared/lib/external_wallet'

import {
  Notification,
  AutoContributeCompletedNotification,
  MonthlyContributionFailedNotification,
  GrantAvailableNotification,
  PendingPublisherVerifiedNotification,
  PendingTipFailedNotification,
  ExternalWalletDisconnectedNotification
} from '../../shared/components/notifications'

function parseGrantId (id: string) {
  const parts = id.split('_')
  return (parts.length > 1 && parts.pop()) || ''
}

function parseGrantDetails (args: string[]) {
  return {
    amount: parseFloat(args[0]) || 0,
    createdAt: parseFloat(args[1]) || null,
    claimableUntil: parseFloat(args[2]) || null,
    expiresAt: null
  }
}

function mapProvider (name: string): ExternalWalletProvider {
  const provider = externalWalletProviderFromString(name.toLocaleLowerCase())
  return provider || 'uphold'
}

enum ExtensionNotificationType {
  AUTO_CONTRIBUTE = 1,
  GRANT = 2,
  GRANT_ADS = 3,
  INSUFFICIENT_FUNDS = 6,
  TIPS_PROCESSED = 8,
  VERIFIED_PUBLISHER = 10,
  PENDING_NOT_ENOUGH_FUNDS = 11,
  GENERAL_LEDGER = 12
}

// Ensures that the specified object literal matches some type |T|
function create<T> (obj: T): T { return obj }

// Converts a notification object coming from the extension API into an instance
// of the |Notification| type. If the object cannot be converted, |null| is
// returned.
export function mapNotification (
  obj: RewardsExtension.Notification
): Notification | null {
  const baseProps = {
    id: obj.id,
    timeStamp: obj.timestamp * 1000 || 0
  }

  switch (obj.type) {
    case ExtensionNotificationType.AUTO_CONTRIBUTE:
      switch (parseInt(obj.args[1], 10)) {
        case 0: // Success
          return create<AutoContributeCompletedNotification>({
            ...baseProps,
            type: 'auto-contribute-completed',
            amount: parseFloat(obj.args[3]) || 0
          })
        case 1: // General error
          return create<MonthlyContributionFailedNotification>({
            ...baseProps,
            type: 'monthly-contribution-failed',
            reason: 'unknown'
          })
        case 15: // Not enough funds
          return create<MonthlyContributionFailedNotification>({
            ...baseProps,
            type: 'monthly-contribution-failed',
            reason: 'insufficient-funds'
          })
      }
      break
    case ExtensionNotificationType.GRANT:
      return create<GrantAvailableNotification>({
        ...baseProps,
        type: 'grant-available',
        grantInfo: {
          type: 'ugp',
          id: parseGrantId(obj.id),
          ...parseGrantDetails(obj.args)
        }
      })
    case ExtensionNotificationType.GRANT_ADS:
      return create<GrantAvailableNotification>({
        ...baseProps,
        type: 'grant-available',
        grantInfo: {
          type: 'ads',
          id: parseGrantId(obj.id),
          ...parseGrantDetails(obj.args)
        }
      })
    case ExtensionNotificationType.INSUFFICIENT_FUNDS:
      return {
        ...baseProps,
        type: 'add-funds'
      }
    case ExtensionNotificationType.TIPS_PROCESSED:
      return {
        ...baseProps,
        type: 'monthly-tip-completed'
      }
    case ExtensionNotificationType.VERIFIED_PUBLISHER:
      return create<PendingPublisherVerifiedNotification>({
        ...baseProps,
        type: 'pending-publisher-verified',
        publisherName: obj.args[0] || ''
      })
    case ExtensionNotificationType.PENDING_NOT_ENOUGH_FUNDS:
      return create<PendingTipFailedNotification>({
        ...baseProps,
        type: 'pending-tip-failed',
        reason: 'insufficient-funds'
      })
    case ExtensionNotificationType.GENERAL_LEDGER:
      switch (obj.args[0]) {
        case 'wallet_disconnected':
          return create<ExternalWalletDisconnectedNotification>({
            ...baseProps,
            type: 'external-wallet-disconnected',
            // The provider is not currently recorded for this notification
            provider: mapProvider('')
          })
      }
      break
  }

  return null
}
