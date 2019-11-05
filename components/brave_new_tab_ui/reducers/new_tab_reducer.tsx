/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Reducer } from 'redux'

// Constants
import { types } from '../constants/new_tab_types'
import { Preferences } from '../api/preferences'
import { Stats } from '../api/stats'
import { PrivateTabData } from '../api/privateTabData'

// API
import * as backgroundAPI from '../api/background'
import * as gridAPI from '../api/topSites/grid'
import { InitialData, InitialRewardsData, PreInitialRewardsData } from '../api/initialData'
import * as bookmarksAPI from '../api/topSites/bookmarks'
import * as dndAPI from '../api/topSites/dnd'
import * as storage from '../storage'
import { getTotalContributions } from '../rewards-utils'

const initialState = storage.load()

function addToDispatchQueue (fn: Function): void {
  window.setTimeout(fn, 0)
}

export const newTabReducer: Reducer<NewTab.State | undefined> = (state: NewTab.State | undefined, action: any) => {
  console.timeStamp('reducer ' + action.type)
  if (state === undefined) {
    console.timeStamp('reducer init')
    state = initialState
  }

  const startingState = state
  const payload = action.payload
  switch (action.type) {
    case types.NEW_TAB_SET_INITIAL_DATA:
      const initialDataPayload = payload as InitialData
      state = {
        ...state,
        initialDataLoaded: true,
        ...initialDataPayload.preferences,
        stats: initialDataPayload.stats,
        ...initialDataPayload.privateTabData,
        topSites: initialDataPayload.topSites
      }
      if (initialDataPayload.preferences.showBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      console.timeStamp('reducer initial data received')
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs.
      // See for example the discussion at:
      // https://stackoverflow.com/questions/36730793/can-i-dispatch-an-action-in-reducer
      // This specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break
    case types.BOOKMARK_ADDED:
      const topSite: NewTab.Site | undefined = state.topSites.find((site) => site.url === payload.url)
      if (topSite) {
        chrome.bookmarks.create({
          title: topSite.title,
          url: topSite.url
        }, () => {
          bookmarksAPI.fetchBookmarkInfo(payload.url)
        })
      }
      break
    case types.BOOKMARK_REMOVED:
      const bookmarkInfo = state.bookmarks[payload.url]
      if (bookmarkInfo) {
        chrome.bookmarks.remove(bookmarkInfo.id, () => {
          bookmarksAPI.fetchBookmarkInfo(payload.url)
        })
      }
      break
    case types.NEW_TAB_SITE_PINNED: {
      const topSiteIndex: number = state.topSites.findIndex((site) => site.url === payload.url)
      const pinnedTopSite: NewTab.Site = Object.assign({}, state.topSites[topSiteIndex], { pinned: true })
      const pinnedTopSites: NewTab.Site[] = state.pinnedTopSites.slice()

      pinnedTopSite.index = topSiteIndex
      pinnedTopSites.push(pinnedTopSite)
      pinnedTopSites.sort((x, y) => x.index - y.index)
      state = {
        ...state,
        pinnedTopSites
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break
    }

    case types.NEW_TAB_SITE_UNPINNED:
      const currentPositionIndex: number = state.pinnedTopSites.findIndex((site) => site.url === payload.url)
      if (currentPositionIndex !== -1) {
        const pinnedTopSites: NewTab.Site[] = state.pinnedTopSites.slice()
        pinnedTopSites.splice(currentPositionIndex, 1)
        state = {
          ...state,
          pinnedTopSites
        }
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break

    case types.NEW_TAB_SITE_IGNORED: {
      const topSiteIndex: number = state.topSites.findIndex((site) => site.url === payload.url)
      const ignoredTopSites: NewTab.Site[] = state.ignoredTopSites.slice()
      ignoredTopSites.push(state.topSites[topSiteIndex])
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: true
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break
    }

    case types.NEW_TAB_UNDO_SITE_IGNORED: {
      const ignoredTopSites: NewTab.Site[] = state.ignoredTopSites.slice()
      ignoredTopSites.pop()
      state = {
        ...state,
        ignoredTopSites,
        showSiteRemovalNotification: false
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break
    }

    case types.NEW_TAB_UNDO_ALL_SITE_IGNORED:
      state = {
        ...state,
        ignoredTopSites: [],
        showSiteRemovalNotification: false
      }
      // Assume 'top sites' data needs changing, so call 'calculate'.
      // TODO(petemill): Starting another dispatch (which happens
      // in `calculateGridSites`) before this reducer is finished
      // is an anti-pattern and could introduce bugs. This
      // specific calculation would be better as a selector at
      // UI render time.
      // We at least schedule to run after the reducer has finished
      // and the resulting new state is available.
      addToDispatchQueue(() => {
        gridAPI.calculateGridSites(state)
      })
      break

    case types.NEW_TAB_HIDE_SITE_REMOVAL_NOTIFICATION:
      state = {
        ...state,
        showSiteRemovalNotification: false
      }
      break

    case types.NEW_TAB_SITE_DRAGGED:
      state = dndAPI.onDraggedSite(state, payload.fromUrl, payload.toUrl)
      break

    case types.NEW_TAB_SITE_DRAG_END:
      state = dndAPI.onDragEnd(state)
      break

    case types.NEW_TAB_BOOKMARK_INFO_AVAILABLE:
      state = bookmarksAPI.updateBookmarkInfo(state, payload.queryUrl, payload.bookmarkTreeNode)
      break

    case types.NEW_TAB_GRID_SITES_UPDATED:
      state = { ...state, gridSites: payload.gridSites }
      break

    case types.NEW_TAB_STATS_UPDATED:
      const stats: Stats = payload.stats
      state = {
        ...state,
        stats
      }
      break

    case types.NEW_TAB_PRIVATE_TAB_DATA_UPDATED:
      const privateTabData = payload as PrivateTabData
      state = {
        ...state,
        useAlternativePrivateSearchEngine: privateTabData.useAlternativePrivateSearchEngine
      }
      break

    case types.NEW_TAB_PREFERENCES_UPDATED:
      const preferences = payload as Preferences
      const shouldChangeBackgroundImage =
        !state.showBackgroundImage && preferences.showBackgroundImage
      state = {
        ...state,
        ...preferences
      }
      if (shouldChangeBackgroundImage) {
        state.backgroundImage = backgroundAPI.randomBackgroundImage()
      }
      break

    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      state = { ...state }
      state.rewardsState.walletCreating = true
      break

    case types.ON_ENABLED_MAIN:
      state = { ...state }
      state.rewardsState.enabledMain = payload.enabledMain
      break

    case types.ON_WALLET_INITIALIZED: {
      const result: NewTab.RewardsResult = payload.result
      state = { ...state }

      switch (result) {
        case NewTab.RewardsResult.WALLET_CORRUPT:
          state.rewardsState.walletCorrupted = true
          break
        case NewTab.RewardsResult.WALLET_CREATED:
          state.rewardsState.walletCreated = true
          state.rewardsState.walletCreateFailed = false
          state.rewardsState.walletCreating = false
          state.rewardsState.walletCorrupted = false
          chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
          break
        case NewTab.RewardsResult.LEDGER_OK:
          state.rewardsState.walletCreateFailed = true
          state.rewardsState.walletCreating = false
          state.rewardsState.walletCreated = false
          state.rewardsState.walletCorrupted = false
          break
      }
      break
    }

    case types.ON_REWARDS_SETTING_SAVE:
      const key = action.payload.key
      const value = action.payload.value

      if (key) {
        state = { ...state }
        state.rewardsState[key] = !!value
        chrome.braveRewards.saveSetting(key, value)
      }
      break

    case types.ON_ADS_ENABLED:
      state = { ...state }
      state.rewardsState.enabledAds = payload.enabled
      break

    case types.ON_ADS_ESTIMATED_EARNINGS:
      state = { ...state }
      state.rewardsState.adsEstimatedEarnings = payload.amount
      break

    case types.ON_BALANCE_REPORTS:
      state = { ...state }
      const reports = payload.reports || {}
      state.rewardsState.totalContribution = getTotalContributions(reports)
      break

    case types.DISMISS_NOTIFICATION:
      state = { ...state }

      const dismissedNotifications = state.rewardsState.dismissedNotifications
      dismissedNotifications.push(payload.id)
      state.rewardsState.dismissedNotifications = dismissedNotifications

      state.rewardsState.grants = state.rewardsState.grants.filter((grant) => {
        return grant.promotionId !== payload.id
      })
      break

    case types.ON_GRANT:
      if (action.payload.properties.status === 1) {
        break
      }

      const promotionId = payload.properties.promotionId
      if (!promotionId) {
        break
      }

      state = { ...state }

      if (!state.rewardsState.dismissedNotifications) {
        state.rewardsState.dismissedNotifications = []
      }

      if (state.rewardsState.dismissedNotifications.indexOf(promotionId) > -1) {
        break
      }

      const hasGrant = state.rewardsState.grants.find((grant: NewTab.GrantRecord) => {
        return grant.promotionId === promotionId
      })
      if (hasGrant) {
        break
      }

      const updatedGrants = state.rewardsState.grants
      updatedGrants.push({
        promotionId: promotionId,
        type: payload.properties.type
      })

      state.rewardsState.grants = updatedGrants
      break

    case types.ON_GRANT_FINISH:
      const properties = payload.properties

      if (properties.status !== 0) {
        break
      }

      state = { ...state }
      const oldNotifications = state.rewardsState.dismissedNotifications

      oldNotifications.push(payload.id)
      state.rewardsState.dismissedNotifications = oldNotifications

      state.rewardsState.grants = state.rewardsState.grants.filter((grant) => {
        return grant.promotionId !== properties.promotionId
      })
      break

    case types.ON_BALANCE:
      state = { ...state }
      state.rewardsState.balance = payload.balance
      break

    case types.ON_WALLET_EXISTS:
      if (!payload.exists || state.rewardsState.walletCreated) {
        break
      }
      state.rewardsState.walletCreated = true
      break

    case types.SET_PRE_INITIAL_REWARDS_DATA:
      const preInitialRewardsDataPayload = payload as PreInitialRewardsData
      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          enabledAds: preInitialRewardsDataPayload.enabledAds,
          adsSupported: preInitialRewardsDataPayload.adsSupported,
          enabledMain: preInitialRewardsDataPayload.enabledMain
        }
      }
      break

    case types.SET_INITIAL_REWARDS_DATA:
      const initialRewardsDataPayload = payload as InitialRewardsData
      const newRewardsState = {
        onlyAnonWallet: initialRewardsDataPayload.onlyAnonWallet,
        balance: initialRewardsDataPayload.balance,
        totalContribution: getTotalContributions(initialRewardsDataPayload.reports),
        adsEstimatedEarnings: initialRewardsDataPayload.adsEstimatedEarnings
      }

      state = {
        ...state,
        rewardsState: {
          ...state.rewardsState,
          ...newRewardsState
        }
      }
      break

    default:
      break
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}

export default newTabReducer
