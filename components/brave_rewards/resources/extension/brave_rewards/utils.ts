/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import BigNumber from 'bignumber.js'
import { getMessage } from './background/api/locale_api'
import { WalletState } from '../../ui/components/walletWrapper'

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

export const getGrant = (grant?: RewardsExtension.GrantInfo, onlyAnonWallet?: boolean) => {
  if (!grant) {
    return grant
  }

  const tokenString = onlyAnonWallet ? getMessage('point') : getMessage('token')
  grant.finishTitle = getMessage('grantFinishTitleUGP')
  grant.finishText = getMessage('grantFinishTextUGP', [tokenString])
  grant.finishTokenTitle = onlyAnonWallet
    ? getMessage('grantFinishPointTitleUGP')
    : getMessage('grantFinishTokenTitleUGP')

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
