/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const convertBalance = (tokens: string, rate: number): string => {
  const tokensNum = parseFloat(tokens)
  if (tokensNum === 0) {
    return '0.00'
  }

  const converted = tokensNum * rate

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const isPublisherVerified = (status: Rewards.PublisherStatus) => {
  return status === 2
}

export const isPublisherConnected = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 1
}

export const isPublisherConnectedOrVerified = (status: Rewards.PublisherStatus) => {
  return status === 2 || status === 1
}

export const isPublisherNotVerified = (status: Rewards.PublisherStatus) => {
  return status === 0
}

export const getWalletStatus = (externalWallet?: RewardsTip.ExternalWallet) => {
  if (!externalWallet) {
    return 'unverified'
  }

  switch (externalWallet.status) {
    case 1:
      return 'connected'
    case 2:
      return 'verified'
    case 3:
      return 'disconnected_unverified'
    case 4:
      return 'disconnected_verified'
    default:
      return 'unverified'
  }
}
