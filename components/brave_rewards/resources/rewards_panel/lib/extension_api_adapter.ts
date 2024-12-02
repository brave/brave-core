/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { Notification } from '../../shared/components/notifications'
import { UserType, userTypeFromString } from '../../shared/lib/user_type'
import { ProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { RewardsSummaryData } from '../../shared/components/wallet_card'
import { OnboardingResult } from '../../shared/components/onboarding'
import { mapNotification } from './notification_adapter'

import {
  ExternalWalletProvider,
  ExternalWalletProviderRegionInfo,
  externalWalletProviderFromString,
  externalWalletFromExtensionData
} from '../../shared/lib/external_wallet'

import {
  EarningsInfo,
  ExchangeInfo,
  Options,
  PublisherInfo,
  Settings
} from './interfaces'

import { optional, Optional } from '../../shared/lib/optional'

// The functions exported from this module wrap the existing |braveRewards|
// and |rewardsNotifications| extension API functions in order to reduce the
// amount of mapping/adapter code found in |extension_host|. As the extension
// APIs are improved, the need for this adapter will diminish.

export function getRewardsBalance () {
  return new Promise<Optional<number>>((resolve) => {
    chrome.braveRewards.fetchBalance((balance) => { resolve(optional(balance)) })
  })
}

export function getSettings () {
  return new Promise<Settings>((resolve) => {
    chrome.braveRewards.getPrefs((prefs) => {
      resolve({
        autoContributeEnabled: prefs.autoContributeEnabled,
        autoContributeAmount: prefs.autoContributeAmount
      })
    })
  })
}

export function getEarningsInfo () {
  return new Promise<EarningsInfo | null>((resolve) => {
    chrome.braveRewards.getAdsAccountStatement((success, statement) => {
      if (success) {
        resolve({
          adsReceivedThisMonth: statement.adsReceivedThisMonth,
          minEarningsLastMonth: statement.minEarningsLastMonth,
          maxEarningsLastMonth: statement.maxEarningsLastMonth,
          minEarningsThisMonth: statement.minEarningsThisMonth,
          maxEarningsThisMonth: statement.maxEarningsThisMonth,
          nextPaymentDate: statement.nextPaymentDate
        })
      } else {
        resolve(null)
      }
    })
  })
}

export function getRewardsParameters () {
  interface Result {
    exchangeInfo: ExchangeInfo
    payoutStatus: Record<string, ProviderPayoutStatus>
    options: Options
  }

  return new Promise<Result>((resolve) => {
    chrome.braveRewards.getRewardsParameters((parameters) => {
      const regionMap = new Map<string, ExternalWalletProviderRegionInfo>()
      for (const key of Object.keys(parameters.walletProviderRegions)) {
        const info = parameters.walletProviderRegions[key]
        if (info) {
          regionMap.set(key, info)
        }
      }

      resolve({
        options: {
          externalWalletRegions: regionMap,
          autoContributeAmounts: parameters.autoContributeChoices,
          vbatDeadline: parameters.vbatDeadline,
          vbatExpired: parameters.vbatExpired
        },
        exchangeInfo: {
          currency: 'USD',
          rate: parameters.rate
        },
        payoutStatus: parameters.payoutStatus
      })
    })
  })
}

export function getSelfCustodyInviteDismissed () {
  return new Promise<boolean>((resolve) => {
    chrome.braveRewards.selfCustodyInviteDismissed(resolve)
  })
}

export function isTermsOfServiceUpdateRequired () {
  return new Promise<boolean>((resolve) => {
    chrome.braveRewards.isTermsOfServiceUpdateRequired(resolve)
  })
}

export function getExternalWalletProviders () {
  return new Promise<ExternalWalletProvider[]>((resolve) => {
    chrome.braveRewards.getExternalWalletProviders((providers) => {
      let list: ExternalWalletProvider[] = []
      for (const name of providers) {
        const provider = externalWalletProviderFromString(name)
        if (provider) {
          list.push(provider)
        }
      }
      resolve(list)
    })
  })
}

export function getExternalWallet () {
  return new Promise((resolve) => {
    chrome.braveRewards.getExternalWallet((wallet) => { resolve(wallet) })
  }).then(externalWalletFromExtensionData)
}

export function getRewardsSummaryData () {
  return new Promise<RewardsSummaryData>((resolve) => {
    const now = new Date()
    const month = now.getMonth() + 1
    const year = now.getFullYear()

    chrome.braveRewards.getBalanceReport(month, year, (balanceReport) => {
      resolve({
        adEarnings: balanceReport.ads,
        autoContributions: balanceReport.contribute,
        oneTimeTips: balanceReport.tips,
        monthlyTips: balanceReport.monthly
      })
    })
  })
}

export function getNotifications () {
  return new Promise<Notification[]>((resolve) => {
    chrome.braveRewards.getAllNotifications((list) => {
      const notifications: Notification[] = []
      const typeSet = new Set<string>()

      for (const obj of list) {
        let notification = mapNotification(obj)

        // Legacy "monthly contribution failure" notifications are keyed on the
        // contribution ID, which can result it duplicate failure notifications.
        // Dedupe them now.
        if (notification &&
            notification.type === 'monthly-contribution-failed' &&
            typeSet.has(notification.type)) {
          notification = null
        }

        if (notification) {
          typeSet.add(notification.type)
          notifications.push(notification)
        } else {
          // If the notification is "invalid", remove it from the browser's
          // notification store.
          chrome.rewardsNotifications.deleteNotification(obj.id)
        }
      }
      resolve(notifications)
    })
  })
}

export function getRewardsEnabled () {
  return new Promise<boolean>((resolve) => {
    chrome.braveRewards.getRewardsEnabled(resolve)
  })
}

export function getUserType () {
  return new Promise<UserType>((resolve) => {
    chrome.braveRewards.getUserType((userType) => {
      resolve(userTypeFromString(userType))
    })
  })
}

export function getPublishersVisitedCount () {
  return new Promise<number>((resolve) => {
    chrome.braveRewards.getPublishersVisitedCount(resolve)
  })
}

export function getDeclaredCountry () {
  return new Promise<string>((resolve) => {
    chrome.braveRewards.getDeclaredCountry(resolve)
  })
}

export function createRewardsWallet (country: string) {
  return new Promise<OnboardingResult>((resolve) => {
    chrome.braveRewards.createRewardsWallet(country, (result) => {
      switch (result) {
        case 'success':
        case 'unexpected-error':
        case 'wallet-generation-disabled':
        case 'country-already-declared':
          resolve(result)
          break
        default:
          resolve('unexpected-error')
          break
      }
    })
  })
}

export function getAvailableCountries () {
  return new Promise<string[]>((resolve) => {
    chrome.braveRewards.getAvailableCountries(resolve)
  })
}

export function getDefaultCountry () {
  return new Promise<string>((resolve) => {
    chrome.braveRewards.getDefaultCountry(resolve)
  })
}

function getMonthlyTipAmount (publisherKey: string) {
  return new Promise<number>((resolve) => {
    chrome.braveRewards.getRecurringTips((result) => {
      for (const item of result.recurringTips) {
        if (item.publisherKey === publisherKey) {
          resolve(item.amount)
          return
        }
      }
      resolve(0)
    })
  })
}

function getTab (tabId: number) {
  return new Promise<chrome.tabs.Tab | null>((resolve) => {
    chrome.tabs.get(tabId, (tab) => { resolve(tab || null) })
  })
}

function parseURL (url: string) {
  try {
    return new URL(url)
  } catch {
    return null
  }
}

function isPublisherURL (url: string) {
  const parsedURL = parseURL(url)
  return parsedURL && /^https?:$/.test(parsedURL.protocol)
}

function iconURL (url: string) {
  if (url) {
    return `chrome://favicon2/?size=64&pageUrl=${encodeURIComponent(url)}`
  }
  return ''
}

function defaultPublisherInfo (url: string): PublisherInfo | null {
  const parsedURL = parseURL(url)
  if (!parsedURL) {
    return null
  }

  return {
    id: parsedURL.hostname,
    name: parsedURL.hostname,
    verified: false,
    icon: iconURL(parsedURL.origin),
    platform: null,
    attentionScore: 0,
    autoContributeEnabled: true,
    monthlyTip: 0,
    supportedWalletProviders: []
  }
}

function getPublisherPlatform (name: string) {
  switch (name) {
    case 'github':
    case 'reddit':
    case 'twitch':
    case 'twitter':
    case 'vimeo':
    case 'youtube':
      return name
  }
  return null
}

export async function getPublisherInfo (tabId: number) {
  const tab = await getTab(tabId)
  if (!tab || !tab.url) {
    return null
  }

  if (!isPublisherURL(tab.url)) {
    return null
  }

  const publisher = await new Promise<any>((resolve) => {
    chrome.braveRewards.getPublisherInfoForTab(tabId, resolve)
  })

  if (!publisher) {
    return defaultPublisherInfo(tab.url)
  }

  const { publisherKey } = publisher

  if (!publisherKey || typeof publisherKey !== 'string') {
    return defaultPublisherInfo(tab.url)
  }

  const supportedWalletProviders: ExternalWalletProvider[] = []
  let verified = true

  switch (Number(publisher.status) || 0) {
    case 0: // NOT_VERIFIED
      verified = false
      break
    case 2: // UPHOLD_VERIFIED
      supportedWalletProviders.push('uphold')
      break
    case 3: // BITFLYER_VERIFIED
      supportedWalletProviders.push('bitflyer')
      break
    case 4: // GEMINI_VERIFIED
      supportedWalletProviders.push('gemini')
      break
  }

  const info: PublisherInfo = {
    id: publisherKey,
    name: String(publisher.name || ''),
    verified,
    icon: iconURL(String(publisher.favIconUrl || publisher.url || '')),
    platform: getPublisherPlatform(String(publisher.provider || '')),
    attentionScore: Number(publisher.percentage) / 100 || 0,
    autoContributeEnabled: !publisher.excluded,
    monthlyTip: await getMonthlyTipAmount(publisherKey),
    supportedWalletProviders
  }

  return info
}

export function onPublisherDataUpdated (callback: () => void) {
  chrome.braveRewards.onPublisherData.addListener(() => { callback() })
}
