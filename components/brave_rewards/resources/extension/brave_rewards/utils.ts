/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

import { getMessage } from './background/api/locale_api'
import { WalletState } from '../../ui/components/walletWrapper'

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

export const getPromotion = (promotion: RewardsExtension.Promotion, onlyAnonWallet: boolean) => {
  if (!promotion) {
    return promotion
  }

  const tokenString = onlyAnonWallet ? getMessage('point') : getMessage('token')
  promotion.finishTitle = getMessage('grantFinishTitleUGP')
  promotion.finishText = getMessage('grantFinishTextUGP', [tokenString])
  promotion.finishTokenTitle = onlyAnonWallet
    ? getMessage('grantFinishPointTitleUGP')
    : getMessage('grantFinishTokenTitleUGP')

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

export const getWalletStatus = (externalWallet?: RewardsExtension.ExternalWallet): WalletState | undefined => {
  if (!externalWallet) {
    return undefined
  }

  switch (externalWallet.status) {
    // ledger::type::WalletStatus::CONNECTED
    case 1:
      return 'connected'
    // ledger::type::WalletStatus::VERIFIED
    case 2:
      return 'verified'
    // ledger::type::WalletStatus::DISCONNECTED_NOT_VERIFIED
    case 3:
      return 'disconnected_unverified'
    // ledger::type::WalletStatus::DISCONNECTED_VERIFIED
    case 4:
      return 'disconnected_verified'
    // ledger::type::WalletStatus::PENDING
    case 5:
      return 'pending'
    default:
      return 'unverified'
  }
}

export const getGreetings = (externalWallet?: RewardsExtension.ExternalWallet) => {
  if (!externalWallet || !externalWallet.userName) {
    return ''
  }

  return getMessage('greetingsVerified', [externalWallet.userName])
}

export const handleExternalWalletLink = (balance: RewardsExtension.Balance, externalWallet?: RewardsExtension.ExternalWallet) => {
  if (!externalWallet) {
    return
  }

  let link = externalWallet.verifyUrl

  if (!externalWallet || (externalWallet && externalWallet.status === 0)) {
    link = 'brave://rewards/#verify'
  }

  if (balance.total < 25 && externalWallet && externalWallet.type === 'uphold') {
    link = externalWallet.loginUrl
  }

  chrome.tabs.create({
    url: link
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
  switch (wallet ? wallet.type : '') {
    case 'uphold' : return 'Uphold'
    case 'bitflyer': return 'bitFlyer'
    default: return ''
  }
}
