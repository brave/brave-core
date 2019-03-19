/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

declare namespace chrome {
  function getVariableValue (variable: string): string
  function setVariableValue (variable: string, value: any): void
  function send (stat: string, args?: any[]): void
}

declare namespace chrome.dns {
  function resolve (hostname: string, callback: any): void
}

declare namespace chrome.braveRewards {
  const createWallet: () => {}
  const donateToSite: (tabId: number, publisherKey: string) => {}
  const getPublisherData: (windowId: number, url: string, faviconUrl: string, publisherBlob: string | undefined) => {}
  const getWalletProperties: () => {}
  const getCurrentReport: () => {}
  const onWalletCreated: {
    addListener: (callback: () => void) => void
  }
  const onPublisherData: {
    addListener: (callback: (windowId: number, publisher: RewardsExtension.Publisher) => void) => void
  }
  const onWalletProperties: {
    addListener: (callback: (properties: RewardsExtension.WalletProperties) => void) => void
  }
  const onCurrentReport: {
    addListener: (callback: (properties: RewardsExtension.Report) => void) => void
  }
  const onGrant: {
    addListener: (callback: (properties: RewardsExtension.GrantResponse) => void) => void
  }
  const onGrantFinish: {
    addListener: (callback: (properties: RewardsExtension.GrantFinish) => void) => void
  }
  const onGrantCaptcha: {
    addListener: (callback: (properties: RewardsExtension.Captcha) => void) => void
  }
  const includeInAutoContribution: (publisherKey: string, excluded: boolean) => {}
  const getGrants: () => {}
  const getGrantCaptcha: (promotionId: string, type: string) => {}
  const solveGrantCaptcha: (solution: string, promotionId: string) => {}
  const getPendingContributionsTotal: (callback: (amount: number) => void) => {}
  const getNonVerifiedSettings: (callback: (nonVerified: boolean) => void) => {}
  const onEnabledMain: {
    addListener: (callback: (enabledMain: boolean) => void) => void
  }
  const getRewardsMainEnabled: (callback: (enabled: boolean) => void) => {}
  const saveAdsSetting: (key: string, value: string) => {}
  const onPendingContributionSaved: {
    addListener: (callback: (result: number) => void) => void
  }
  const onWalletFailed: {
    addListener: (callback: () => void) => void
  }
  const getACEnabled: (callback: (enabled: boolean) => void) => {}
  const onPublisherListNormalized: {
    addListener: (callback: (properties: RewardsExtension.PublisherNormalized[]) => void) => void
  }
  const onExcludedSitesChanged: {
    addListener: (callback: (properties: RewardsExtension.ExcludedSitesChanged) => void) => void
  }
  const onRecurringDonations: {
    addListener: (callback: (donations: Record<string, number>[]) => void) => {}
  }
  const saveSetting: (key: string, value: string) => {}
  const getRecurringDonations: (callback: (donations: RewardsExtension.RecurringDonation) => void) => {}
  const saveRecurringDonation: (publisherKey: string, newAmount: string) => {}
  const removeRecurringDonation: (publisherKey: string) => {}
  const getPublisherBanner: (publisherKey: string, callback: (banner: RewardsExtension.PublisherBanner) => void) => {}
}

declare namespace chrome.rewardsNotifications {
  const addNotification: (type: number, args: string[], id: string) => {}
  const deleteNotification: (id: string) => {}
  const deleteAllNotifications: () => {}
  const getNotification: (id: string) => {}

  const onNotificationAdded: {
    addListener: (callback: (id: string, type: number, timestamp: number, args: string[]) => void) => void
  }
  const onNotificationDeleted: {
    addListener: (callback: (id: string, type: number, timestamp: number) => void) => void
  }
  const onAllNotificationsDeleted: {
    addListener: (callback: () => void) => void
  }
  const onGetNotification: {
    addListener: (callback: (id: string, type: number, timestamp: number) => void) => void
  }
}

type BlockTypes = 'ads' | 'trackers' | 'httpUpgradableResources' | 'javascript' | 'fingerprinting'

interface BlockDetails {
  blockType: BlockTypes
  tabId: number
  subresource: string
}

interface BlockDetails {
  blockType: BlockTypes
  tabId: number
  subresource: string
}
declare namespace chrome.tabs {
  const setAsync: any
  const getAsync: any
}

declare namespace chrome.windows {
  const getAllAsync: any
}

declare namespace chrome.braveShields {
  const onBlocked: {
    addListener: (callback: (detail: BlockDetails) => void) => void
    emit: (detail: BlockDetails) => void
  }

  const allowScriptsOnce: any
  const javascript: any
  const plugins: any
}

declare namespace chrome.braveTheme {
  type ThemeType = 'Light' | 'Dark'
  type ThemeTypeCallback = (themeType: ThemeType) => void
  const getBraveThemeType: (themeType: ThemeTypeCallback) => void
  const onBraveThemeTypeChanged: {
    addListener: (callback: ThemeTypeCallback) => void
  }
}
