/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

declare namespace chrome {
  function getVariableValue (variable: string): string
  function setVariableValue (variable: string, value: any): void
  function send (stat: string, args?: any[]): void
}

declare namespace chrome.dns {
  function resolve (hostname: string, callback: any): void
}

declare namespace chrome.settingsPrivate {
  // See chromium definition at
  // https://chromium.googlesource.com/chromium/src.git/+/master/chrome/common/extensions/api/settings_private.idl
  enum PrefType {
    BOOLEAN = 'BOOLEAN',
    NUMBER = 'NUMBER',
    STRING = 'STRING',
    URL = 'URL',
    LIST = 'LIST',
    DICTIONARY = 'DICTIONARY'
  }

  type PrefBooleanValue = {
    type: PrefType.BOOLEAN,
    value: boolean
  }
  type SettingsNumberValue = {
    type: PrefType.NUMBER,
    value: number
  }
  type SettingsStringValue = {
    type: PrefType.STRING,
    value: string
  }
  type PrefDictValue = {
    type: PrefType.DICTIONARY
    value: Object
  }
  // TODO(petemill): implement other types as needed

  type PrefObject = {
    key: string
  } & (PrefBooleanValue | PrefDictValue | SettingsNumberValue | SettingsStringValue)

  type GetPrefCallback = (pref: PrefObject) => void
  function getPref (key: string, callback: GetPrefCallback): void

  type SetPrefCallback = (success: boolean) => void
  function setPref (key: string, value: any, pageId?: string | null, callback?: SetPrefCallback): void
  function setPref (key: string, value: any, callback?: SetPrefCallback): void

  type GetAllPrefsCallback = (prefs: PrefObject[]) => void
  function getAllPrefs (callback: GetAllPrefsCallback): void

  type GetDefaultZoomCallback = (zoom: number) => void
  function getDefaultZoom (callback: GetDefaultZoomCallback): void

  type SetDefaultZoomCallback = (success: boolean) => void
  function setDefaultZoom (zoom: number, callback?: SetDefaultZoomCallback): void

  const onPrefsChanged: {
    addListener: (callback: (prefs: PrefObject[]) => void) => void
  }
}

declare namespace chrome.braveRewards {
  const isSupported: (callback: (supported: boolean) => void) => {}
  const isUnsupportedRegion: (callback: (unsupportedRegion: boolean) => void) => {}

  interface RewardsWallet {
    paymentId: string
    geoCountry: string
  }

  const onRewardsWalletUpdated: {
    addListener: (callback: () => void) => void
  }

  const createRewardsWallet: (country: string, callback: (result?: string) => void) => void
  const getAvailableCountries: (callback: (countries: string[]) => void) => void
  const getDeclaredCountry: (callback: (country: string) => void) => void
  const getUserType: (callback: (userType: string) => void) => void
  const getPublishersVisitedCount: (callback: (count: number) => void) => void
  const getRewardsParameters: (callback: (properties: RewardsExtension.RewardsParameters) => void) => {}
  const updateMediaDuration: (tabId: number, publisherKey: string, duration: number, firstVisit: boolean) => {}
  const getPublisherInfo: (publisherKey: string, callback: (result: RewardsExtension.Result, properties: RewardsExtension.PublisherInfo) => void) => {}
  const getPublisherPanelInfo: (publisherKey: string, callback: (result: RewardsExtension.Result, properties: RewardsExtension.PublisherInfo) => void) => {}

  const setPublisherIdForTab: (tabId: number, publisherId: string) => void
  const getPublisherInfoForTab: (tabId: number, callback: (publisher?: RewardsExtension.PublisherInfo) => void) => void

  const savePublisherInfo: (windowId: number, mediaType: string, url: string, publisherKey: string, publisherName: string, favIconUrl: string, callback: (result: RewardsExtension.Result) => void) => {}
  const tipSite: (tabId: number, publisherKey: string, entryPoint: RewardsExtension.TipDialogEntryPoint) => {}
  const tipUser: (tabId: number, mediaType: string, url: string, publisherKey: string, publisherName: string, publisherScreenName: string, favIconUrl: string, postId: string, postTimestamp: string, postText: string) => {}
  const getPublisherData: (windowId: number, url: string, faviconUrl: string, publisherBlob: string | undefined) => {}
  const getBalanceReport: (month: number, year: number, callback: (properties: RewardsExtension.BalanceReport) => void) => {}
  const onPublisherData: {
    addListener: (callback: (windowId: number, publisher: RewardsExtension.Publisher) => void) => void
  }
  const onPromotions: {
    addListener: (callback: (result: RewardsExtension.Result, promotions: RewardsExtension.Promotion[]) => void) => void
  }
  const onPromotionFinish: {
    addListener: (callback: (result: RewardsExtension.Result, promotion: RewardsExtension.Promotion) => void) => void
  }
  const includeInAutoContribution: (publisherKey: string, exclude: boolean) => {}
  const fetchPromotions: (callback: (promotions: RewardsExtension.Promotion[]) => void) => {}
  const claimPromotion: (promotionId: string, callback: (properties: RewardsExtension.Captcha) => void) => {}
  const attestPromotion: (promotionId: string, solution: string, callback: (result: number, promotion?: RewardsExtension.Promotion) => void) => {}
  const getPendingContributionsTotal: (callback: (amount: number) => void) => {}
  const onAdsEnabled: {
    addListener: (callback: (enabled: boolean) => void) => void
  }
  const getRewardsEnabled: (callback: (enabled: boolean) => void) => {}
  const getAdsEnabled: (callback: (enabled: boolean) => void) => {}
  const getAdsSupported: (callback: (supported: boolean) => void) => {}
  const getAdsAccountStatement: (callback: (success: boolean, adsAccountStatement: NewTab.AdsAccountStatement) => void) => {}
  const getWalletExists: (callback: (exists: boolean) => void) => {}
  const saveAdsSetting: (key: string, value: string) => {}
  const setAutoContributeEnabled: (enabled: boolean) => {}
  const onPendingContributionSaved: {
    addListener: (callback: (result: number) => void) => void
  }
  const getACEnabled: (callback: (enabled: boolean) => void) => {}
  const onPublisherListNormalized: {
    addListener: (callback: (properties: RewardsExtension.PublisherNormalized[]) => void) => void
  }
  const onExcludedSitesChanged: {
    addListener: (callback: (properties: RewardsExtension.ExcludedSitesChanged) => void) => void
  }
  const saveSetting: (key: string, value: string) => {}
  const getRecurringTips: (callback: (tips: RewardsExtension.RecurringTips) => void) => {}
  const saveRecurringTip: (publisherKey: string, newAmount: string) => {}
  const removeRecurringTip: (publisherKey: string) => {}
  const onRecurringTipSaved: {
    addListener: (callback: (success: boolean) => void) => void
  }
  const onRecurringTipRemoved: {
    addListener: (callback: (success: boolean) => void) => void
  }
  const refreshPublisher: (publisherKey: string, callback: (status: number, publisherKey: string) => void) => {}
  const getAllNotifications: (callback: (list: RewardsExtension.Notification[]) => void) => {}
  const getInlineTippingPlatformEnabled: (key: string, callback: (enabled: boolean) => void) => {}
  const fetchBalance: (callback: (balance: RewardsExtension.Balance) => void) => {}
  const onReconcileComplete: {
    addListener: (callback: (result: number, type: number) => void) => void
  }

  const getExternalWallet: (callback: (wallet?: RewardsExtension.ExternalWallet) => void) => {}

  const getExternalWalletProviders: (callback: (providers: string[]) => void) => void

  const onExternalWalletConnected: {
    addListener: (callback: () => void) => void
  }

  const onExternalWalletLoggedOut: {
    addListener: (callback: () => void) => void
  }

  const openRewardsPanel: () => void

  const showRewardsTour: () => void

  const showGrantCaptcha: (grantId: string) => void

  const onUnblindedTokensReady: {
    addListener: (callback: () => void) => void
  }

  const onCompleteReset: {
    addListener: (callback: (properties: { success: boolean }) => void) => void
  }
  const initialized: {
    addListener: (callback: (result: RewardsExtension.Result) => void) => void
  }
  const isInitialized: (callback: (initialized: boolean) => void) => {}

  const getScheduledCaptchaInfo: (
    callback: (scheduledCaptcha: RewardsExtension.ScheduledCaptcha) => void
  ) => void

  const updateScheduledCaptchaResult: (result: boolean) => void

  const enableAds: () => void

  interface RewardsPrefs {
    adsEnabled: boolean
    adsPerHour: number
    autoContributeEnabled: boolean
    autoContributeAmount: number
  }

  const getPrefs: (callback: (prefs: RewardsPrefs) => void) => void
  const updatePrefs: (prefs: Partial<RewardsPrefs>) => void
}

declare namespace chrome.braveTalk {
  const isSupported: (callback: (supported: boolean) => void) => {}
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

declare namespace chrome.greaselion {
  const isGreaselionExtension: (id: string, callback: (valid: boolean) => void) => {}
}

declare namespace chrome.braveToday {
  const onClearHistory: {
    addListener: (callback: () => any) => void
  }
  const getHostname: (callback: (hostname: string) => any) => void
  const getRegionUrlPart: (callback: (regionURLPart: string) => any) => void
}

type BlockTypes = 'shieldsAds' | 'trackers' | 'httpUpgradableResources' | 'javascript' | 'fingerprinting'

declare namespace chrome.tabs {
  const setAsync: any
  const getAsync: any
}

declare namespace chrome.windows {
  const getAllAsync: any
}

declare namespace chrome.braveShields {
  const addSiteCosmeticFilter: (origin: string, cssSelector: string) => void
  const openFilterManagementPage: () => void
}

declare namespace chrome.test {
  const sendMessage: (message: string) => {}
}
