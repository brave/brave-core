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

export function truncatePayoutDate(payoutDate: number | Date) {
  if (typeof payoutDate === 'number') {
    payoutDate = new Date(payoutDate)
  }

  // Round next payment date down to midnight local time.
  return new Date(
    payoutDate.getFullYear(),
    payoutDate.getMonth(),
    payoutDate.getDate())
}

const pendingDaysFormatter = new Intl.NumberFormat(undefined, {
  style: 'unit',
  unit: 'day',
  unitDisplay: 'long',
  maximumFractionDigits: 0
})

export function getDaysUntilPayout(nextPayoutDate: number | Date) {
  nextPayoutDate = truncatePayoutDate(nextPayoutDate)
  const now = Date.now()

  // Only show pending days when payment date is within the current month.
  if (nextPayoutDate.getMonth() !== new Date(now).getMonth()) {
    return ''
  }

  const delta = nextPayoutDate.getTime() - now
  const days = Math.ceil(delta / 24 / 60 / 60 / 1000)
  if (days < 1) {
    return ''
  }

  return pendingDaysFormatter.format(days)
}
