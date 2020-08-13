/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'
import { Reducer } from 'redux'
import { setBadgeText } from '../browserAction'
import { isPublisherConnectedOrVerified } from '../../utils'

const getTabKey = (id: number) => {
  return `key_${id}`
}

const updateBadgeTextAllWindows = (windows: chrome.windows.Window[], state?: RewardsExtension.State) => {
  if (!state || windows.length === 0) {
    return
  }

  windows.forEach((window => {
    const tabKey = getTabKey(window.id)
    const publishers: Record<string, RewardsExtension.Publisher> = state.publishers
    const publisher = publishers[tabKey]

    if (!publisher || !window.tabs) {
      return
    }

    let tab = window.tabs.find((tab) => tab.active)

    if (!tab) {
      return
    }

    setBadgeText(state, isPublisherConnectedOrVerified(publisher.status), tab.id)
  }))

}

const handledByGreaselion = (url: URL) => {
  if (!url) {
    return false
  }

  return url.hostname.endsWith('.youtube.com') || url.hostname === 'youtube.com'
}

export const rewardsPanelReducer: Reducer<RewardsExtension.State | undefined> = (state: RewardsExtension.State, action: any) => {
  if (!state) {
    return
  }
  const payload = action.payload
  switch (action.type) {
    case types.TOGGLE_ENABLE_MAIN: {
      if (state.initializing && state.enabledMain) {
        break
      }

      state = { ...state }
      const key = 'enabledMain'
      const enable = action.payload.enable
      state.initializing = true

      state[key] = enable
      chrome.braveRewards.saveSetting(key, enable ? '1' : '0')
      break
    }
    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      state = { ...state }
      state.walletCreating = true
      state.walletCreateFailed = false
      state.walletCreated = false
      state.walletCorrupted = false
      state.initializing = true
      break
    case types.WALLET_CREATED: {
      state = { ...state }
      state.initializing = false
      state.walletCreated = true
      state.walletCreateFailed = false
      state.walletCreating = false
      state.walletCorrupted = false
      state.enabledMain = true
      chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
      chrome.storage.local.get(['is_dismissed'], function (result) {
        if (result && result['is_dismissed'] === 'false') {
          chrome.browserAction.setBadgeText({
            text: ''
          })
          chrome.storage.local.remove(['is_dismissed'])
        }
      })
      break
    }
    case types.WALLET_CREATION_FAILED: {
      state = { ...state }
      const result: RewardsExtension.Result = payload.result
      state.initializing = false
      if (result === RewardsExtension.Result.WALLET_CORRUPT) {
        state.walletCorrupted = true
      } else {
        state.walletCreateFailed = true
        state.walletCreating = false
        state.walletCreated = false
        state.walletCorrupted = false
      }
      break
    }
    case types.ON_TAB_RETRIEVED: {
      const tab: chrome.tabs.Tab = payload.tab
      if (
        !tab ||
        !tab.id ||
        !tab.url ||
        tab.incognito ||
        !tab.active ||
        !state.walletCreated ||
        !state.enabledMain
      ) {
        break
      }

      const tabKey = getTabKey(tab.id)
      const publishers: Record<string, RewardsExtension.Publisher> = state.publishers
      const publisher = publishers[tabKey]
      const validKey = publisher && publisher.publisherKey && publisher.publisherKey.length > 0

      if (!publisher || (publisher.tabUrl !== tab.url || !validKey)) {
        // Invalid publisher for tab, re-fetch publisher.
        if (!handledByGreaselion(new URL(tab.url))) {
          chrome.braveRewards.getPublisherData(
            tab.id,
            tab.url,
            tab.favIconUrl || '',
            payload.publisherBlob || '')
        }

        if (publisher) {
          delete publishers[tabKey]
        }

        publishers[tabKey] = {
          tabUrl: tab.url,
          tabId: tab.id
        }

      } else if (publisher &&
                 publisher.tabUrl === tab.url &&
                 (publisher.tabId !== tab.id || payload.activeTabIsLoadingTriggered) &&
                 validKey) {
        // Valid match and either is a different tab with the same url,
        // or the same tab but it has been unloaded and re-loaded.
        // Set state.
        setBadgeText(state, isPublisherConnectedOrVerified(publisher.status), tab.id)
        publishers[tabKey].tabId = tab.id
      }

      state = {
        ...state,
        publishers
      }
      break
    }
    case types.ON_PUBLISHER_DATA: {
      const publisher = payload.publisher
      let publishers: Record<string, RewardsExtension.Publisher> = state.publishers
      const tabKey = getTabKey(payload.windowId)

      if (publisher && !publisher.publisherKey) {
        delete publishers[tabKey]
      } else {
        publishers[tabKey] = { ...publishers[tabKey], ...publisher }
        const newPublisher = publishers[tabKey]

        if (newPublisher.tabId) {
          setBadgeText(state, isPublisherConnectedOrVerified(newPublisher.status), newPublisher.tabId)
        }
      }

      state = {
        ...state,
        publishers
      }
      break
    }
    case types.ON_BALANCE_REPORT: {
      state = { ...state }
      state.balanceReport = payload.report
      break
    }
    case types.ON_NOTIFICATION_ADDED: {
      if (!payload || payload.id === '') {
        return
      }

      const id: string = payload.id
      let notifications: Record<string, RewardsExtension.Notification> = state.notifications

      // Array check for previous version of state types
      // (https://github.com/brave/brave-browser/issues/4344)
      if (!notifications || Array.isArray(notifications)) {
        notifications = {}
      }

      notifications[id] = {
        id: id,
        type: payload.type,
        timestamp: payload.timestamp,
        args: payload.args
      }

      state = {
        ...state,
        notifications
      }

      if (state.currentNotification === undefined) {
        state.currentNotification = id
      }

      setBadgeText(state)
      break
    }
    case types.DELETE_NOTIFICATION: {
      let id = payload.id
      if (id.startsWith('n_')) {
        id = id.toString().replace('n_', '')
      }

      chrome.rewardsNotifications.deleteNotification(id)
      break
    }
    case types.ON_NOTIFICATION_DELETED: {
      if (!payload || payload.id === '') {
        return
      }

      let id = payload.id
      let notifications: Record<number, RewardsExtension.Notification> = state.notifications
      if (!notifications[id]) {
        id = `n_${id}`
      }

      delete notifications[id]

      if (state.currentNotification === id) {
        let current: number | undefined = undefined
        Object.keys(state.notifications).forEach((key: string) => {
          if (
            current === undefined ||
            !notifications[current] ||
            notifications[key].timestamp > notifications[current].timestamp
          ) {
            current = notifications[key].id
          }

        })

        state.currentNotification = current
      }

      setBadgeText(state)

      if (state.currentNotification === undefined) {
        updateBadgeTextAllWindows(payload.windows, state)
      }

      state = {
        ...state,
        notifications
      }
      break
    }
    case types.INCLUDE_IN_AUTO_CONTRIBUTION: {
      let publisherKey = payload.publisherKey
      let exclude = payload.exclude
      chrome.braveRewards.includeInAutoContribution(publisherKey, exclude)
      break
    }
    case types.ON_PENDING_CONTRIBUTIONS_TOTAL: {
      state = { ...state }
      state.pendingContributionTotal = payload.amount
      break
    }
    case types.ON_ENABLED_MAIN: {
      state = { ...state }
      const enabled = payload.enabledMain
      if (enabled == null) {
        break
      }

      if (state.enabledMain && !enabled) {
        state = storage.defaultState
        state.enabledMain = false
        state.walletCreated = true
        break
      }

      state.enabledMain = enabled
      break
    }
    case types.ON_ENABLED_AC: {
      state = { ...state }
      if (payload.enabled == null) {
        break
      }
      state.enabledAC = payload.enabled
      break
    }
    case types.ON_PUBLISHER_LIST_NORMALIZED: {
      const list = payload.properties
      let publishers: Record<string, RewardsExtension.Publisher> = state.publishers

      if (!list || list.length === 0) {
        break
      }

      for (const key in publishers) {
        let publisher = publishers[key]
        const updated = list.find((newPublisher: RewardsExtension.PublisherNormalized) =>
          newPublisher.publisherKey === publisher.publisherKey)

        if (updated) {
          publisher.status = updated.status
          publisher.percentage = updated.percentage
          publisher.excluded = false
        } else {
          publisher.percentage = 0
        }
      }

      state = {
        ...state,
        publishers
      }
      break
    }
    case types.ON_EXCLUDED_SITES_CHANGED: {
      if (!payload.properties) {
        break
      }

      const publisherKey: string = payload.properties.publisherKey
      const excluded: boolean = payload.properties.excluded

      let publishers: Record<string, RewardsExtension.Publisher> = state.publishers

      for (const key in publishers) {
        let publisher = publishers[key]

        if (publisher.publisherKey === publisherKey) {
          publisher.excluded = !!excluded
        } else if (publisherKey === '-1') {
          publisher.excluded = false
        }
      }

      state = {
        ...state,
        publishers
      }
      break
    }
    case types.ON_SETTING_SAVE: {
      state = { ...state }
      const key = action.payload.key
      const value = action.payload.value
      if (key) {
        state[key] = value
        chrome.braveRewards.saveSetting(key, value)
      }
      break
    }
    case types.REMOVE_RECURRING_TIP: {
      let publisherKey = payload.publisherKey
      if (publisherKey == null) {
        break
      }
      chrome.braveRewards.removeRecurringTip(publisherKey)
      break
    }
    case types.SAVE_RECURRING_TIP: {
      let newAmount = payload.newAmount
      let publisherKey = payload.publisherKey

      if (newAmount < 0 ||
          isNaN(newAmount) ||
          publisherKey == null) {
        break
      }

      chrome.braveRewards.saveRecurringTip(publisherKey, newAmount)
      break
    }
    case types.ON_RECURRING_TIPS: {
      state = { ...state }
      state.recurringTips = payload.result.recurringTips
      break
    }
    case types.ON_PUBLISHER_BANNER: {
      if (!payload.banner || !payload.banner.publisherKey) {
        break
      }

      state = { ...state }
      if (!state.tipAmounts) {
        state.tipAmounts = {}
      }
      state.tipAmounts[payload.banner.publisherKey] = payload.banner.amounts
      break
    }
    case types.ON_PUBLISHER_STATUS_REFRESHED: {
      const publisherKey = payload.publisherKey
      if (publisherKey) {
        let publishers: Record<string, RewardsExtension.Publisher> = state.publishers
        for (const key in publishers) {
          let publisher = publishers[key]
          if (publisher.publisherKey === publisherKey) {
            publisher.status = payload.status
          }
        }
        state = {
          ...state,
          publishers
        }
      }
      break
    }
    case types.ON_ALL_NOTIFICATIONS: {
      const list: RewardsExtension.Notification[] = payload.list

      if (!list) {
        state = {
          ...state,
          notifications: {},
          currentNotification: undefined
        }
        setBadgeText(state)
        break
      }

      let notifications = {}
      let id = ''
      list.forEach((notification: RewardsExtension.Notification) => {
        id = notification.id
        notifications[notification.id] = {
          id: notification.id,
          type: notification.type,
          timestamp: notification.timestamp,
          args: notification.args
        }
      })

      state = {
        ...state,
        notifications
      }

      const found = list.find((notification: RewardsExtension.Notification) => {
        if (!notification || !state) {
          return false
        }

        return notification.id === state.currentNotification
      })

      if (id && (state.currentNotification === undefined || !found)) {
        state.currentNotification = id
      }

      setBadgeText(state)
      break
    }

    case types.ON_INIT: {
      const tabs: chrome.tabs.Tab[] = payload.tabs

      if (!tabs) {
        break
      }

      const publishers: Record<string, RewardsExtension.Publisher> = state.publishers

      tabs.forEach((tab) => {
        const tabId = tab.id
        if (!tabId) {
          return
        }

        const tabKey = getTabKey(tabId)
        const publisher = publishers[tabKey]

        if (!publisher || publisher.tabId !== tabId) {
          return
        }

        setBadgeText(state, isPublisherConnectedOrVerified(publisher.status), publisher.tabId)
      })

      break
    }
    case types.ON_BALANCE: {
      state = { ...state }
      state.balance = payload.balance
      break
    }
    case types.ON_EXTERNAL_WALLET: {
      state = { ...state }
      state.externalWallet = payload.wallet
      break
    }
    case types.ON_ANON_WALLET_STATUS: {
      state = { ...state }

      state.walletCorrupted = false
      state.walletCreating = false
      state.walletCreated = false
      state.walletCreateFailed = false

      if (payload.result === RewardsExtension.Result.WALLET_CORRUPT) {
        state.walletCorrupted = true
      } else if (payload.result === RewardsExtension.Result.WALLET_CREATED) {
        state.walletCreated = true
      }
      break
    }
    case types.ON_REWARDS_PARAMETERS: {
      state = {
        ...state,
        parameters: payload.parameters
      }
      break
    }
    case types.ON_ALL_NOTIFICATIONS_DELETED: {
      state = {
        ...state,
        notifications: {},
        currentNotification: undefined
      }
      setBadgeText(state)
      break
    }
    case types.ON_COMPLETE_RESET: {
      if (payload.success) {
        return undefined
      }
      break
    }
    case types.INITIALIZED: {
      state = {
        ...state,
        initializing: false
      }
      break
    }
    case types.WALLET_EXISTS: {
      state = {
        ...state,
        walletCreated: payload.exists
      }
      break
    }
  }
  return state
}
