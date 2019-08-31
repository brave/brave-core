/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'
import { getMessage } from './background/api/locale_api'

export const convertBalance = (tokens: string, rates: Record<string, number> | undefined, currency: string = 'USD'): string => {
  const tokensNum = parseFloat(tokens)
  if (tokensNum === 0 || !rates || !rates[currency]) {
    return '0.00'
  }

  const converted = tokensNum * rates[currency]

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const convertProbiToFixed = (probi: string, places: number = 1) => {
  const result = new BigNumber(probi).dividedBy('1e18').toFixed(places, BigNumber.ROUND_DOWN)

  if (result === 'NaN') {
    return '0.0'
  }

  return result
}

export const getGrants = (grants?: RewardsExtension.Grant[]) => {
  if (!grants) {
    return []
  }

  return grants.map((grant: RewardsExtension.Grant) => {
    return {
      tokens: convertProbiToFixed(grant.probi),
      expireDate: new Date(grant.expiryTime * 1000).toLocaleDateString(),
      type: grant.type || 'ugp'
    }
  })
}

export const getGrant = (grant?: RewardsExtension.GrantInfo) => {
  if (!grant) {
    return grant
  }

  grant.finishTitle = getMessage('grantFinishTitleUGP')
  grant.finishText = getMessage('grantFinishTextUGP')
  grant.finishTokenTitle = getMessage('grantFinishTokenTitleUGP')

  if (grant.type === 'ads') {
    grant.expiryTime = 0
    grant.finishTitle = getMessage('grantFinishTitleAds')
    grant.finishText = getMessage('grantFinishTextAds')
    grant.finishTokenTitle = getMessage('grantFinishTokenTitleAds')
  }

  return grant
}

export const isPublisherVerified = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 2
}

export const isPublisherConnected = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 1
}

export const isPublisherConnectedOrVerified = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 2 || status === 1
}

export const isPublisherNotVerified = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 0
}
