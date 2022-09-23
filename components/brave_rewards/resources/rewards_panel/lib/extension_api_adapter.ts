/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Notification } from '../../shared/components/notifications'
import { GrantInfo } from '../../shared/lib/grant_info'
import { ProviderPayoutStatus } from '../../shared/lib/provider_payout_status'
import { RewardsSummaryData } from '../../shared/components/wallet_card'
import { mapNotification } from './notification_adapter'

import {
  ExternalWalletProvider,
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

// The functions exported from this module wrap the existing |braveRewards|
// and |rewardsNotifications| extension API functions in order to reduce the
// amount of mapping/adapter code found in |extension_host|. As the extension
// APIs are improved, the need for this adapter will diminish.

export function getRewardsBalance () {
  return new Promise<number>((resolve) => {
    chrome.braveRewards.fetchBalance((balance) => { resolve(balance.total) })
  })
}

export function getSettings () {
  return new Promise<Settings>((resolve) => {
    chrome.braveRewards.getPrefs((prefs) => {
      resolve({
        adsPerHour: prefs.adsPerHour,
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
          earningsLastMonth: statement.earningsLastMonth,
          earningsThisMonth: statement.earningsThisMonth,
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
      resolve({
        options: {
          autoContributeAmounts: parameters.autoContributeChoices
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

export function getExternalWalletProviders () {
  return new Promise<ExternalWalletProvider[]>((resolve) => {
    // The extension API currently does not support retrieving a list of
    // external wallet providers. Instead, use the `getExternalWallet` function
    // to retrieve the "currently selected" provider.
    chrome.braveRewards.getExternalWallet((_, wallet) => {
      const provider = wallet && externalWalletProviderFromString(wallet.type)
      resolve(provider ? [provider] : [])
    })
  })
}

export function getExternalWallet () {
  return new Promise((resolve) => {
    chrome.braveRewards.getExternalWallet((_, wallet) => { resolve(wallet) })
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
        monthlyTips: balanceReport.monthly,
        pendingTips: 0
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

function promotionToGrant (promotion: RewardsExtension.Promotion): GrantInfo {
  return {
    id: promotion.promotionId,
    type: promotion.type === 1 ? 'ads' : 'ugp',
    amount: promotion.amount,
    createdAt: promotion.createdAt * 1000 || null,
    claimableUntil: promotion.claimableUntil * 1000 || null,
    expiresAt: promotion.expiresAt * 1000 || null
  }
}

type GrantsUpdatedCallback = (grants: GrantInfo[]) => void
let grantsUpdatedCallbacks: GrantsUpdatedCallback[] = []

chrome.braveRewards.onPromotions.addListener((result, promotions) => {
  const grants = promotions.map(promotionToGrant)
  for (const callback of grantsUpdatedCallbacks) {
    callback(grants)
  }
})

export function onGrantsUpdated (callback: (grants: GrantInfo[]) => void) {
  grantsUpdatedCallbacks.push(callback)
}

export function getGrants () {
  return new Promise<GrantInfo[]>((resolve) => {
    chrome.braveRewards.fetchPromotions((promotions) => {
      resolve(promotions.map(promotionToGrant))
    })
  })
}

export function getRewardsEnabled () {
  return new Promise<boolean>((resolve) => {
    chrome.braveRewards.getRewardsEnabled(resolve)
  })
}

export function onRewardsEnabled (callback: () => void) {
  chrome.braveRewards.onRewardsWalletUpdated.addListener(() => { callback() })
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

function defaultPublisherInfo (url: string) {
  const parsedURL = parseURL(url)
  if (!parsedURL) {
    return null
  }

  return {
    id: parsedURL.hostname,
    name: parsedURL.hostname,
    icon: origin ? `chrome://favicon/size/64@1x/${parsedURL.origin}` : '',
    platform: null,
    registered: false,
    attentionScore: 0,
    autoContributeEnabled: true,
    monthlyTip: 0,
    supportedWalletProviders: []
  }
}

function isGreaselionURL (url: string) {
  const parsedURL = parseURL(url)
  if (!parsedURL) {
    return false
  }

  const hosts = [
    'github.com',
    'reddit.com',
    'twitch.tv',
    'twitter.com',
    'vimeo.com',
    'youtube.com'
  ]

  const { hostname } = parsedURL
  return hosts.some((h) => hostname.endsWith(`.${h}`) || hostname === h)
}

export async function fetchPublisherInfo (tabId: number) {
  const tab = await getTab(tabId)
  if (!tab || !tab.url) {
    return
  }

  const { url } = tab

  // Publisher info for "Greaselion" domains is managed by extension content
  // scripts that execute within the context of the tab. We do not need to
  // explicitly request publisher data for these domains.
  if (isGreaselionURL(url)) {
    return
  }

  if (isPublisherURL(url)) {
    const favicon = tab.favIconUrl || ''
    chrome.braveRewards.getPublisherData(tabId, url, favicon, '')
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
  let registered = true

  switch (Number(publisher.status) || 0) {
    case 0: // NOT_VERIFIED
      registered = false
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

  const iconPath = String(publisher.favIconUrl || publisher.url || '')

  const info: PublisherInfo = {
    id: publisherKey,
    name: String(publisher.name || ''),
    icon: iconPath ? `chrome://favicon/size/64@1x/${iconPath}` : '',
    platform: getPublisherPlatform(String(publisher.provider || '')),
    registered,
    attentionScore: Number(publisher.percentage) / 100 || 0,
    autoContributeEnabled: !publisher.excluded,
    monthlyTip: await getMonthlyTipAmount(publisherKey),
    supportedWalletProviders
  }

  return info
}

export function onPublisherDataUpdated (callback: () => void) {
  chrome.braveRewards.onPublisherData.addListener(() => {
    // The background script may not have updated its Redux store at the point
    // when this callback is executed. Unfortunatley, we don't currently have a
    // way to know when the update has finished and must rely on a short delay.
    setTimeout(() => { callback() }, 200)
  })
}
