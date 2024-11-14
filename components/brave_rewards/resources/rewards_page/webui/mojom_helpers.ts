/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as mojom from './mojom'
import { AdType } from '../lib/app_state'

import {
  ExternalWalletProvider,
  externalWalletProviderFromString
} from '../../shared/lib/external_wallet'

import {
  Notification,
  ExternalWalletDisconnectedNotification
} from '../../shared/components/notifications'

// Converts a mojo Time value to a JS time ms value.
export function convertMojoTime(time: any) {
  return (Number(time?.internalValue) / 1000 - Date.UTC(1601, 0, 1)) || 0
}

// Ensures that the specified object literal matches some type |T|
function create<T> (obj: T): T { return obj }

// Converts a notification object coming from the extension API into an instance
// of the |Notification| type. If the object cannot be converted, |null| is
// returned.
export function mapNotification (
  obj: mojom.RewardsNotification
): Notification | null {
  const baseProps = {
    id: obj.id,
    timeStamp: convertMojoTime(obj.timestamp)
  }

  switch (obj.type) {
    case mojom.RewardsNotificationType.kTipsProcessed:
      return {
        ...baseProps,
        type: 'monthly-tip-completed'
      }
    case mojom.RewardsNotificationType.kGeneral:
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
      }
      break
  }

  return null
}

// Converts an AdType value into a mojo enum value.
export function convertAdType(adType: AdType) {
  switch (adType) {
    case 'new-tab-page': return mojom.AdType.kNewTabPageAd
    case 'notification': return mojom.AdType.kNotificationAd
    case 'search-result': return mojom.AdType.kSearchResultAd
    case 'inline-content': return mojom.AdType.kInlineContentAd
  }
}

// Converts a mojo PublisherStatus to a list of wallet providers.
export function walletProvidersFromPublisherStatus(
  value: number
) : ExternalWalletProvider[] {
  switch (value) {
    case mojom.PublisherStatus.BITFLYER_VERIFIED:
      return ['bitflyer']
    case mojom.PublisherStatus.GEMINI_VERIFIED:
      return ['gemini']
    case mojom.PublisherStatus.UPHOLD_VERIFIED:
      return ['uphold']
  }
  return []
}
