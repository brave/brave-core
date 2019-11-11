/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/new_tab_types'
import { Preferences } from '../api/preferences'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'
import { InitialData, InitialRewardsData, PreInitialRewardsData } from '../api/initialData'

export const bookmarkAdded = (url: string) => action(types.BOOKMARK_ADDED, {
  url
})

export const bookmarkRemoved = (url: string) => action(types.BOOKMARK_REMOVED, {
  url
})

export const sitePinned = (url: string) => action(types.NEW_TAB_SITE_PINNED, {
  url
})

export const siteUnpinned = (url: string) => action(types.NEW_TAB_SITE_UNPINNED, {
  url
})

export const siteIgnored = (url: string) => action(types.NEW_TAB_SITE_IGNORED, {
  url
})

export const undoSiteIgnored = (url: string) => action(types.NEW_TAB_UNDO_SITE_IGNORED, {
  url
})

export const undoAllSiteIgnored = (url: string) => action(types.NEW_TAB_UNDO_ALL_SITE_IGNORED, {
  url
})

export const siteDragged = (fromUrl: string, toUrl: string, dragRight: boolean) => action(types.NEW_TAB_SITE_DRAGGED, {
  fromUrl,
  toUrl,
  dragRight
})

export const siteDragEnd = (url: string, didDrop: boolean) => action(types.NEW_TAB_SITE_DRAG_END, {
  url,
  didDrop
})

export const onHideSiteRemovalNotification = () => action(types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION)

export const bookmarkInfoAvailable = (queryUrl: string, bookmarkTreeNode: NewTab.Bookmark) => action(types.NEW_TAB_BOOKMARK_INFO_AVAILABLE, {
  queryUrl,
  bookmarkTreeNode
})

export const gridSitesUpdated = (gridSites: NewTab.Site[]) => action(types.NEW_TAB_GRID_SITES_UPDATED, {
  gridSites
})

export const statsUpdated = (stats: Stats) =>
  action(types.NEW_TAB_STATS_UPDATED, {
    stats
  })

export const privateTabDataUpdated = (data: PrivateTabData) =>
  action(types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED, data)

export const preferencesUpdated = (preferences: Preferences) =>
  action(types.NEW_TAB_PREFERENCES_UPDATED, preferences)

export const setInitialData = (initialData: InitialData) =>
  action(types.NEW_TAB_SET_INITIAL_DATA, initialData)

export const createWallet = () => action(types.CREATE_WALLET, {})

export const onEnabledMain = (enabledMain: boolean) => action(types.ON_ENABLED_MAIN, {
  enabledMain
})

export const onAdsEnabled = (enabled: boolean) => action(types.ON_ADS_ENABLED, {
  enabled
})

export const onRewardsSettingSave = (key: string, value: any) => action(types.ON_REWARDS_SETTING_SAVE, {
  key,
  value
})

export const onWalletInitialized = (result: NewTab.RewardsResult) => action(types.ON_WALLET_INITIALIZED, {
  result
})

export const onAdsEstimatedEarnings = (amount: number) => action(types.ON_ADS_ESTIMATED_EARNINGS, {
  amount
})

export const onBalanceReports = (reports: Record<string, NewTab.RewardsReport>) => action(types.ON_BALANCE_REPORTS, {
  reports
})

export const onPromotions = (result: number, promotions: NewTab.Promotion[]) => action(types.ON_PROMOTIONS, {
  result,
  promotions
})

export const dismissNotification = (id: string) => action(types.DISMISS_NOTIFICATION, {
  id
})

export const onBalance = (balance: NewTab.RewardsBalance) => action(types.ON_BALANCE, {
  balance
})

export const onWalletExists = (exists: boolean) => action(types.ON_WALLET_EXISTS, {
  exists
})

export const setInitialRewardsData = (initialRewardsData: InitialRewardsData) => action(types.SET_INITIAL_REWARDS_DATA, initialRewardsData)

export const setPreInitialRewardsData = (preInitialRewardsData: PreInitialRewardsData) => action(types.SET_PRE_INITIAL_REWARDS_DATA, preInitialRewardsData)
