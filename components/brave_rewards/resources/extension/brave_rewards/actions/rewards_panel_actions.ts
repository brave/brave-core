/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_panel_types'

export const onTabRetrieved = (tab: chrome.tabs.Tab, activeTabIsLoadingTriggered: boolean, publisherBlob: string = 'ignore') => action(types.ON_TAB_RETRIEVED, {
  tab,
  activeTabIsLoadingTriggered,
  publisherBlob
})

export const onPublisherData = (windowId: number, publisher: RewardsExtension.Publisher) => action(types.ON_PUBLISHER_DATA, {
  windowId,
  publisher
})

export const onRewardsParameters = (parameters: RewardsExtension.RewardsParameters) => action(types.ON_REWARDS_PARAMETERS, {
  parameters
})

export const onBalanceReport = (report: RewardsExtension.BalanceReport) => action(types.ON_BALANCE_REPORT, {
  report
})

export const onNotificationAdded = (id: string, type: number, timestamp: number, args: string[]) => action(types.ON_NOTIFICATION_ADDED, {
  id,
  type,
  timestamp,
  args
})

export const onNotificationDeleted = (id: string, type: number, timestamp: number, windows: chrome.windows.Window[]) => action(types.ON_NOTIFICATION_DELETED, {
  id,
  timestamp,
  type,
  windows
})

export const deleteNotification = (id: string) => action(types.DELETE_NOTIFICATION, {
  id
})

export const includeInAutoContribution = (publisherKey: string, exclude: boolean) => action(types.INCLUDE_IN_AUTO_CONTRIBUTION, {
  publisherKey,
  exclude
})

export const fetchPromotions = () => action(types.FETCH_PROMOTIONS)

export const onPromotions = (result: RewardsExtension.Result, promotions: RewardsExtension.Promotion[]) => action(types.ON_PROMOTIONS, {
  result,
  promotions
})
export const promotionFinished = (result: RewardsExtension.Result, promotion: RewardsExtension.Promotion) => action(types.ON_PROMOTION_FINISH, {
  result,
  promotion
})

export const onClaimPromotion = (properties: RewardsExtension.Captcha) => action(types.ON_CLAIM_PROMOTION, {
  properties
})

export const resetPromotion = (promotionId: string) => action(types.RESET_PROMOTION, {
  promotionId
})

export const deletePromotion = (promotionId: string) => action(types.DELETE_PROMOTION, {
  promotionId
})

export const OnPendingContributionsTotal = (amount: number) => action(types.ON_PENDING_CONTRIBUTIONS_TOTAL, {
  amount
})

export const onEnabledAC = (enabled: boolean) => action(types.ON_ENABLED_AC, {
  enabled
})

export const onShouldShowOnboarding = (showOnboarding: boolean) => action(types.ON_SHOULD_SHOW_ONBOARDING, {
  showOnboarding
})

export const saveOnboardingResult = (result: 'opted-in' | 'dismissed') => action(types.SAVE_ONBOARDING_RESULT, {
  result
})

export const onPublisherListNormalized = (properties: RewardsExtension.PublisherNormalized[]) =>
  action(types.ON_PUBLISHER_LIST_NORMALIZED, {
    properties
  })

export const onExcludedSitesChanged = (properties: RewardsExtension.ExcludedSitesChanged) =>
  action(types.ON_EXCLUDED_SITES_CHANGED, {
    properties
  })

export const onSettingSave = (key: string, value: any) => action(types.ON_SETTING_SAVE, {
  key,
  value
})

export const saveRecurringTip = (publisherKey: string, newAmount: number) => action(types.SAVE_RECURRING_TIP, {
  publisherKey,
  newAmount
})

export const removeRecurringTip = (publisherKey: string) => action(types.REMOVE_RECURRING_TIP, {
  publisherKey
})

export const onRecurringTips = (result: RewardsExtension.RecurringTips) => action(types.ON_RECURRING_TIPS, {
  result
})

export const onPublisherBanner = (banner: RewardsExtension.PublisherBanner) =>
  action(types.ON_PUBLISHER_BANNER, {
    banner
  })

export const refreshPublisher = (status: number, publisherKey: string) => action(types.ON_PUBLISHER_STATUS_REFRESHED, {
  status,
  publisherKey
})

export const onAllNotifications = (list: RewardsExtension.Notification[]) => action(types.ON_ALL_NOTIFICATIONS, {
  list
})

export const init = (tabs: chrome.tabs.Tab[]) => action(types.ON_INIT, {
  tabs
})

export const onBalance = (balance: RewardsExtension.Balance) => action(types.ON_BALANCE, {
  balance
})

export const onExternalWallet = (wallet: RewardsExtension.ExternalWallet) => action(types.ON_EXTERNAL_WALLET, {
  wallet
})

export const onAllNotificationsDeleted = () => action(types.ON_ALL_NOTIFICATIONS_DELETED)

export const onCompleteReset = (success: boolean) => action(types.ON_COMPLETE_RESET, {
  success
})

export const initialized = () => action(types.INITIALIZED)

export const updatePrefs = (prefs: Partial<chrome.braveRewards.RewardsPrefs>) =>
  action(types.UPDATE_PREFS, { prefs })

export const onGetPrefs = (prefs: chrome.braveRewards.RewardsPrefs) =>
  action(types.ON_GET_PREFS, { prefs })
