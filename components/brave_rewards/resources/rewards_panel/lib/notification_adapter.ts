/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { externalWalletProviderFromString } from '../../shared/lib/external_wallet'

import {
  Notification,
  AutoContributeCompletedNotification,
  MonthlyContributionFailedNotification,
  GrantAvailableNotification,
  ExternalWalletDisconnectedNotification,
  UpholdBATNotAllowedNotification,
  UpholdInsufficientCapabilitiesNotification
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

enum ExtensionNotificationType {
  AUTO_CONTRIBUTE = 1,
  GRANT = 2,
  GRANT_ADS = 3,
  TIPS_PROCESSED = 8,
  GENERAL = 12
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
        case 15: // Not enough funds
          return create<MonthlyContributionFailedNotification>({
            ...baseProps,
            type: 'monthly-contribution-failed',
            reason: 'unknown'
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
    case ExtensionNotificationType.TIPS_PROCESSED:
      return {
        ...baseProps,
        type: 'monthly-tip-completed'
      }
    case ExtensionNotificationType.GENERAL:
      switch (obj.args[0]) {
        case 'wallet_disconnected': {
          const provider = externalWalletProviderFromString(obj.args[1] || '')
          if (!provider) {
            return null
          }
          return create<ExternalWalletDisconnectedNotification>({
            ...baseProps,
            type: 'external-wallet-disconnected',
            provider
          })
        }
        case 'uphold_bat_not_allowed': {
          return create<UpholdBATNotAllowedNotification>({
            ...baseProps,
            type: 'uphold-bat-not-allowed',
            provider: 'uphold'
          })
        }
        case 'uphold_insufficient_capabilities': {
          return create<UpholdInsufficientCapabilitiesNotification>({
            ...baseProps,
            type: 'uphold-insufficient-capabilities',
            provider: 'uphold'
          })
        }
      }
      break
  }

  return null
}
