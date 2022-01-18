/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export type GrantType = 'ads' | 'ugp'

export interface GrantInfo {
  id: string
  type: GrantType
  amount: number
  createdAt: number | null
  expiresAt: number | null
  claimableUntil: number | null
}

const daysToClaimFormatter = new Intl.NumberFormat(undefined, {
  style: 'unit',
  unit: 'day',
  unitDisplay: 'long',
  maximumFractionDigits: 0
})

const monthFormatter = new Intl.DateTimeFormat(undefined, {
  month: 'long'
})

export function formatGrantMonth (grantInfo: GrantInfo) {
  const created = new Date(grantInfo.createdAt || Date.now())
  const grantMonth = new Date(created.getFullYear(), created.getMonth() - 1)
  return monthFormatter.format(grantMonth)
}

export function formatGrantDaysToClaim (grantInfo: GrantInfo) {
  const { claimableUntil } = grantInfo
  if (!claimableUntil) {
    return ''
  }

  const days = Math.ceil((claimableUntil - Date.now()) / 1000 / 60 / 60 / 24)
  if (days <= 0) {
    return ''
  }

  return daysToClaimFormatter.format(days)
}
