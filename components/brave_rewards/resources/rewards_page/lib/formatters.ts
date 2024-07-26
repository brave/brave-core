/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

const earningsEstimateFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 2,
  maximumFractionDigits: 2
})

const earningsRangeFormatter = new Intl.NumberFormat(undefined, {
  minimumFractionDigits: 3,
  maximumFractionDigits: 3
})

export function formatEarningsRange(startRange: number, endRange: number) {
  const parts = earningsRangeFormatter.formatRangeToParts(startRange, endRange)
  for (const part of parts) {
    if (part.type === 'literal' && part.value === '–') {
      part.value = ' – '
    }
  }
  return parts.map((part) => part.value).join('')
}

export function formatEarningsEstimate(startRange: number, endRange: number) {
  const mid = (startRange + endRange) / 2
  return `≈ ${ earningsEstimateFormatter.format(mid) }`
}
