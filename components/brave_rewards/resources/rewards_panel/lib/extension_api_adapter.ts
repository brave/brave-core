/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Store } from 'webext-redux'

import { Notification } from '../../shared/components/notifications'
import { RewardsSummaryData } from '../../shared/components/wallet_card'
import { mapNotification } from './notification_adapter'

import {
  ExternalWallet,
  ExternalWalletProvider,
  ExternalWalletStatus,
  externalWalletProviderFromString
} from '../../shared/lib/external_wallet'

import {
  EarningsInfo,
  ExchangeInfo,
  GrantInfo,
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

export function getExternalWalletProviders () {
  return new Promise<ExternalWalletProvider[]>((resolve) => {
    // The extension API currently does not support retrieving a list of
    // external wallet providers.
    chrome.braveRewards.getExternalWallet((_, wallet) => {
      const provider = externalWalletProviderFromString(wallet.type)
      resolve(provider ? [provider] : [])
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
        }
      })
    })
  })
}

const externalWalletLoginURLs = new Map<ExternalWalletProvider, string>()

export function getExternalWalletLoginURL (provider: ExternalWalletProvider) {
  return new Promise<string>((resolve) => {
    resolve(externalWalletLoginURLs.get(provider) || '')
  })
}

export function getExternalWallet () {
  function mapStatus (status: number): ExternalWalletStatus | null {
    switch (status) {
      case 1: // CONNECTED
      case 2: // VERIFIED
        return 'verified'
      case 3: // DISCONNECTED_NOT_VERIFIED
      case 4: // DISCONNECTED_VERIFIED
        return 'disconnected'
      case 5: // PENDING
        return 'pending'
    }
    return null
  }

  return new Promise<ExternalWallet | null>((resolve) => {
    chrome.braveRewards.getExternalWallet((_, wallet) => {
      const provider = externalWalletProviderFromString(wallet.type)
      const status = mapStatus(wallet.status)

      if (!provider || !status) {
        resolve(null)
      } else {
        externalWalletLoginURLs.set(provider, wallet.loginUrl)

        resolve({
          provider,
          status,
          username: wallet.userName,
          links: {
            account: wallet.accountUrl,
            addFunds: wallet.addUrl,
            completeVerification: wallet.verifyUrl
          }
        })
      }
    })
  })
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

type GrantsUpdatedCallback = (grants: GrantInfo[]) => void

let grantResolver: GrantsUpdatedCallback | null = null
let grantPromise: Promise<GrantInfo[]> | null = null
let grantsUpdatedCallbacks: GrantsUpdatedCallback[] = []

chrome.braveRewards.onPromotions.addListener((result, promotions) => {
  const grants: GrantInfo[] = []
  for (const obj of promotions) {
    const source = obj.type === 1 ? 'ads' : 'ugp'
    grants.push({
      id: obj.promotionId,
      source,
      amount: obj.amount,
      expiresAt: obj.expiresAt * 1000 || null
    })
  }

  // If a caller of |getGrants| is currently waiting on a result, resolve the
  // associated promise.
  if (grantResolver) {
    grantResolver(grants)
  }

  for (const callback of grantsUpdatedCallbacks) {
    callback(grants)
  }
})

export function onGrantsUpdated (callback: (grants: GrantInfo[]) => void) {
  grantsUpdatedCallbacks.push(callback)
}

export function getGrants () {
  if (!grantPromise) {
    grantPromise = new Promise<GrantInfo[]>((r) => { grantResolver = r })
    chrome.braveRewards.fetchPromotions()
  }
  return grantPromise
}

export function getRewardsEnabled () {
  return new Promise<boolean>((resolve) => {
    // Currently, we must use the |showShowOnboarding| function to infer whether
    // the user has enabled rewards.
    chrome.braveRewards.shouldShowOnboarding((showOnboarding) => {
      resolve(!showOnboarding)
    })
  })
}

export function onRewardsEnabled (callback: () => void) {
  // The extension does not currently support an |onRewardsEnabled| function.
  chrome.braveRewards.onAdsEnabled.addListener(() => { callback() })
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

// The mapping from WebContents (via tab) to publisher ID is currently
// maintained in the Rewards extension's background script and is accessed using
// message passing, implemented as a Redux store proxy using the "webext-redux"
// library. Eventually, we should move the mapping into the browser and remove
// this proxy.
const backgroundReduxStore = new Store({ portName: 'REWARDSPANEL' })

async function getPublisherFromBackgroundState (tabId: number) {
  await backgroundReduxStore.ready()

  const state = backgroundReduxStore.getState()
  if (!state) {
    return {}
  }
  const { rewardsPanelData } = state
  if (!rewardsPanelData) {
    return {}
  }
  const { publishers } = rewardsPanelData
  if (!publishers) {
    return {}
  }
  return publishers[`key_${tabId}`] || {}
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
    monthlyContribution: 0,
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
    case 'twitter':
    case 'youtube':
    case 'twitch':
    case 'reddit':
    case 'vimeo':
    case 'github':
      return name
  }
  return null
}

export async function getPublisherInfo (tabId: number) {
  const tab = await getTab(tabId)
  if (!tab) {
    return null
  }

  const publisher = await getPublisherFromBackgroundState(tabId)
  const { publisherKey } = publisher

  if (!publisherKey || typeof publisherKey !== 'string') {
    const url = tab.url || ''
    if (isPublisherURL(url)) {
      return defaultPublisherInfo(url)
    }
    return null
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
    monthlyContribution: await getMonthlyTipAmount(publisherKey),
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
