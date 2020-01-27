/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'

import { getMessage } from './background/api/locale_api'
import { WalletState } from '../../ui/components/walletWrapper'

export const convertBalance = (tokens: number, rates: Record<string, number> | undefined, currency: string = 'USD'): string => {
  if (tokens === 0 || !rates || !rates[currency]) {
    return '0.00'
  }

  const converted = tokens * rates[currency]

  if (isNaN(converted)) {
    return '0.00'
  }

  return converted.toFixed(2)
}

export const formatConverted = (converted: string, currency: string = 'USD'): string | null => {
  return `${converted} ${currency}`
}

export const handleContributionAmount = (amount: string) => {
  let result = '0.0'
  const amountSplit = amount.split('.')
  if (amountSplit && amountSplit[0].length > 18) {
    const result = new BigNumber(amount).dividedBy('1e18').toFixed(1, BigNumber.ROUND_UP)

    return result
  } else {
    result = parseFloat(amount).toFixed(1)
  }

  if (result === 'NaN') {
    return '0.0'
  }

  return result
}

export const generatePromotions = (promotions?: RewardsExtension.Promotion[]) => {
  if (!promotions) {
    return []
  }

  let claimedPromotions = promotions.filter((promotion: Rewards.Promotion) => {
    return promotion.status === 4 // PromotionStatus::FINISHED
  })

  const typeUGP = 0
  return claimedPromotions.map((promotion: RewardsExtension.Promotion) => {
    return {
      amount: promotion.amount,
      expiresAt: new Date(promotion.expiresAt).toLocaleDateString(),
      type: promotion.type || typeUGP
    }
  })
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

export const getWalletStatus = (externalWallet?: RewardsExtension.ExternalWallet): WalletState => {
  if (!externalWallet) {
    return 'unverified'
  }

  switch (externalWallet.status) {
    // ledger::WalletStatus::CONNECTED
    case 1:
      return 'connected'
    // ledger::WalletStatus::VERIFIED
    case 2:
      return 'verified'
    // ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED
    case 3:
      return 'disconnected_unverified'
    // ledger::WalletStatus::DISCONNECTED_VERIFIED
    case 4:
      return 'disconnected_verified'
    default:
      return 'unverified'
  }
}

export const getUserName = (externalWallet?: RewardsExtension.ExternalWallet) => {
  if (!externalWallet) {
    return ''
  }

  return externalWallet.userName
}

export const handleUpholdLink = (link: string, externalWallet?: RewardsExtension.ExternalWallet) => {
  if (!externalWallet || (externalWallet && externalWallet.status === 0)) {
    link = 'brave://rewards/#verify'
  }

  chrome.tabs.create({
    url: link
  })
}

export const getExternalWallet = (actions: any, externalWallet?: RewardsExtension.ExternalWallet, open: boolean = false) => {
  chrome.braveRewards.getExternalWallet('uphold', (result: number, wallet: RewardsExtension.ExternalWallet) => {
    // EXPIRED TOKEN
    if (result === 24) {
      getExternalWallet(actions, externalWallet, open)
      return
    }

    actions.onExternalWallet(wallet)

    if (open && wallet.verifyUrl) {
      handleUpholdLink(wallet.verifyUrl)
    }
  })
}

export const onVerifyClick = (actions: any, externalWallet?: RewardsExtension.ExternalWallet) => {
  if (!externalWallet || externalWallet.verifyUrl) {
    getExternalWallet(actions, externalWallet, true)
    return
  }

  handleUpholdLink(externalWallet.verifyUrl)
}

export const getClaimedPromotions = (promotions: RewardsExtension.Promotion[]) => {
  return promotions.filter((promotion: RewardsExtension.Promotion) => {
    return promotion.status === 4 // PromotionStatus::FINISHED
  })
}
