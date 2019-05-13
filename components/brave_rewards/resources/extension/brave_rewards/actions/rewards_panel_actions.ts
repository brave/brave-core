/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_panel_types'

export const createWallet = () => action(types.CREATE_WALLET, {})

export const onWalletInitialized = (result: RewardsExtension.Result) => action(types.ON_WALLET_INITIALIZED, {
  result
})

export const onTabId = (tabId: number | undefined) => action(types.ON_TAB_ID, {
  tabId
})

export const onTabRetrieved = (tab: chrome.tabs.Tab, publisherBlob: string, activeTabIsLoadingTriggered: boolean) => action(types.ON_TAB_RETRIEVED, {
  tab,
  publisherBlob,
  activeTabIsLoadingTriggered
})

export const onPublisherData = (windowId: number, publisher: RewardsExtension.Publisher) => action(types.ON_PUBLISHER_DATA, {
  windowId,
  publisher
})

export const getWalletProperties = () => action(types.GET_WALLET_PROPERTIES, {})

export const onWalletProperties = (properties: RewardsExtension.WalletProperties) => action(types.ON_WALLET_PROPERTIES, {
  properties
})

export const getCurrentReport = () => action(types.GET_CURRENT_REPORT, {})

export const onCurrentReport = (properties: RewardsExtension.Report) => action(types.ON_CURRENT_REPORT, {
  properties
})

export const onNotificationAdded = (id: string, type: number, timestamp: number, args: string[]) => action(types.ON_NOTIFICATION_ADDED, {
  id,
  type,
  timestamp,
  args
})

export const onNotificationDeleted = (id: string, type: number, timestamp: number) => action(types.ON_NOTIFICATION_DELETED, {
  id,
  timestamp,
  type
})

export const deleteNotification = (id: string) => action(types.DELETE_NOTIFICATION, {
  id
})

export const includeInAutoContribution = (publisherKey: string, excluded: boolean) => action(types.INCLUDE_IN_AUTO_CONTRIBUTION, {
  publisherKey,
  excluded
})

export const getGrants = () => action(types.GET_GRANTS)

export const onGrant = (properties: RewardsExtension.GrantResponse) => action(types.ON_GRANT, {
  properties
})

export const getGrantCaptcha = (promotionId?: string) => action(types.GET_GRANT_CAPTCHA, {
  promotionId
})

export const onGrantCaptcha = (captcha: RewardsExtension.Captcha) => action(types.ON_GRANT_CAPTCHA, {
  captcha
})

export const solveGrantCaptcha = (x: number, y: number) => action(types.SOLVE_GRANT_CAPTCHA, {
  x,
  y
})

export const onGrantFinish = (properties: RewardsExtension.GrantFinish) => action(types.ON_GRANT_FINISH, {
  properties
})

export const onResetGrant = () => action(types.ON_GRANT_RESET)

export const onDeleteGrant = () => action(types.ON_GRANT_DELETE)

export const OnPendingContributionsTotal = (amount: number) => action(types.ON_PENDING_CONTRIBUTIONS_TOTAL, {
  amount
})

export const onEnabledMain = (enabledMain: boolean) => action(types.ON_ENABLED_MAIN, {
  enabledMain
})

export const onEnabledAC = (enabled: boolean) => action(types.ON_ENABLED_AC, {
  enabled
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

export const refreshPublisher = (verified: boolean, publisherKey: string) => action(types.ON_PUBLISHER_STATUS_REFRESHED, {
  verified,
  publisherKey
})

export const onAllNotifications = (list: RewardsExtension.Notification[]) => action(types.ON_ALL_NOTIFICATIONS, {
  list
})

export const init = (tabs: chrome.tabs.Tab[]) => action(types.ON_INIT, {
  tabs
})
