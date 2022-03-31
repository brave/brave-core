/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

import { lookupExternalWalletProviderName } from '../../shared/lib/external_wallet'
import { getMessage } from './background/api/locale_api'

export const convertBalance = (tokens: number, rate: number): string => {
  if (tokens === 0) {
    return '0.00'
  }

  const converted = tokens * rate

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const handleContributionAmount = (amount: string) => {
  let result = '0.000'
  const amountSplit = amount.split('.')
  if (amountSplit && amountSplit[0].length > 18) {
    const result = new BigNumber(amount).dividedBy('1e18').toFixed(3, BigNumber.ROUND_UP)

    return result
  } else {
    result = parseFloat(amount).toFixed(3)
  }

  if (result === 'NaN') {
    return '0.000'
  }

  return result
}

export const getPromotion = (promotion: RewardsExtension.Promotion) => {
  if (!promotion) {
    return promotion
  }

  const tokenString = getMessage('token')
  promotion.finishTitle = getMessage('grantFinishTitleUGP')
  promotion.finishText = getMessage('grantFinishTextUGP', [tokenString])
  promotion.finishTokenTitle = getMessage('grantFinishTokenTitleUGP')

  if (promotion.type === 1) { // Rewards.PromotionTypes.ADS
    promotion.expiresAt = 0
    promotion.finishTitle = getMessage('grantFinishTitleAds')
    promotion.finishText = getMessage('grantFinishTextAds')
    promotion.finishTokenTitle = getMessage('grantFinishTokenTitleAds')
  }

  return promotion
}

export const isPublisherVerified = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status > 1
}

export const isPublisherConnected = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 1
}

export const isPublisherConnectedOrVerified = (status?: RewardsExtension.PublisherStatus) => {
  // Any non-zero publisher status indicates that they are either connected or
  // verified
  return Boolean(status)
}

export const isPublisherNotVerified = (status?: RewardsExtension.PublisherStatus) => {
  if (status === undefined) {
    return false
  }

  return status === 0
}

export const handleExternalWalletLink = (balance: RewardsExtension.Balance, externalWallet?: RewardsExtension.ExternalWallet) => {
  let link = ''

  if (!externalWallet || (externalWallet && externalWallet.status === 0)) {
    link = 'brave://rewards/#verify'
  } else {
    link = externalWallet.verifyUrl
  }

  chrome.tabs.create({
    url: link
  }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
}

export const getExternalWallet = (actions: any, externalWallet?: RewardsExtension.ExternalWallet) => {
  chrome.braveRewards.getExternalWallet((result: number, wallet: RewardsExtension.ExternalWallet) => {
    // EXPIRED TOKEN
    if (result === 24) {
      getExternalWallet(actions, externalWallet)
      return
    }

    actions.onExternalWallet(wallet)
  })
}

export const getWalletProviderName = (wallet?: RewardsExtension.ExternalWallet) => {
  return lookupExternalWalletProviderName(wallet ? wallet.type : '')
}
