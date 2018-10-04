/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

export const convertBalance = (tokens: number, rates: Record<string, number> | undefined, currency: string = 'USD'): number => {
  if (tokens === 0 || !rates || !rates[currency]) {
    return 0
  }

  const converted = tokens * rates[currency]

  if (isNaN(converted)) {
    return 0
  }

  return parseFloat(converted.toFixed(2))
}

export const formatConverted = (converted: number, currency: string = 'USD'): string | null => {
  if (isNaN(converted) || converted < 0) {
    return null
  }

  return `${converted.toFixed(2)} ${currency}`
}

export const convertProbiToDouble = (probi: string) => {
  return new BigNumber(probi.toString()).dividedBy('1e18').toNumber()
}
