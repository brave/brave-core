/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { ExternalWalletProvider } from '../../shared/lib/external_wallet'

export type ProviderPayoutStatus = 'off' | 'processing' | 'complete'

export function getProviderPayoutStatus (
  payoutStatus: Record<string, ProviderPayoutStatus>,
  walletProvider: ExternalWalletProvider | null
): ProviderPayoutStatus {
  const key = walletProvider || 'unverified'
  return payoutStatus[key] || 'off'
}
