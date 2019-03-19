/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/rewards_panel_types'

export const createWallet = () => action(types.CREATE_WALLET, {})

export const onWalletCreated = () => action(types.ON_WALLET_CREATED, {})

export const onWalletCreateFailed = () => action(types.ON_WALLET_CREATE_FAILED, {})

export const onTabId = (tabId: number | undefined) => action(types.ON_TAB_ID, {
  tabId
})

export const onTabRetrieved = (tab: chrome.tabs.Tab, publisherBlob: string) => action(types.ON_TAB_RETRIEVED, {
  tab,
  publisherBlob
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

export const saveRecurringDonation = (publisherKey: string, newAmount: number) => action(types.SAVE_RECURRING_DONATION, {
  publisherKey,
  newAmount
})

export const removeRecurringContribution = (publisherKey: string) => action(types.REMOVE_RECURRING_DONATION, {
  publisherKey
})

export const onRecurringDonations = (result: RewardsExtension.RecurringDonation) => action(types.ON_RECURRING_DONATIONS, {
  result
})

export const onPublisherBanner = (banner: RewardsExtension.PublisherBanner) =>
  action(types.ON_PUBLISHER_BANNER, {
    banner
  })
