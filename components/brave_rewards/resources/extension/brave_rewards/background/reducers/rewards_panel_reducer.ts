/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'
import { setBadgeText } from '../browserAction'
import { isPublisherConnectedOrVerified } from '../../utils'

const getWindowId = (id: number) => {
  return `id_${id}`
}

export const rewardsPanelReducer = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
    setBadgeText(state)
  }
  const payload = action.payload
  switch (action.type) {
    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      state = { ...state }
      state.walletCreating = true
      state.walletCreateFailed = false
      state.walletCreated = false
      state.walletCorrupted = false
      break
    case types.ON_WALLET_INITIALIZED: {
      const result: RewardsExtension.Result = payload.result
      state = { ...state }
      if (result === RewardsExtension.Result.WALLET_CREATED) {
        state.walletCreated = true
        state.walletCreateFailed = false
        state.walletCreating = false
        state.walletCorrupted = false
        chrome.braveRewards.saveAdsSetting('adsEnabled', 'true')
        chrome.storage.local.get(['is_dismissed'], function (result) {
          if (result && result['is_dismissed'] === 'false') {
            chrome.browserAction.setBadgeText({
              text: ''
            })
            chrome.storage.local.remove(['is_dismissed'])
          }
        })
      } else if (result === RewardsExtension.Result.WALLET_CORRUPT) {
        state.walletCorrupted = true
      } else if (result !== RewardsExtension.Result.LEDGER_OK) {
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
        !tab.url ||
        tab.incognito ||
        !tab.active ||
        !state.walletCreated ||
        !state.enabledMain
      ) {
        break
      }

      const id = getWindowId(tab.windowId)
      const publishers: Record<string, RewardsExtension.Publisher> = state.publishers
      const publisher = publishers[id]
      const validKey = publisher && publisher.publisher_key && publisher.publisher_key.length > 0

      if (!publisher || (publisher.tabUrl !== tab.url || !validKey)) {
        // Invalid publisher for tab, re-fetch publisher.
        chrome.braveRewards.getPublisherData(
          tab.windowId,
          tab.url,
          tab.favIconUrl || '',
          payload.publisherBlob || '')

        if (publisher) {
          delete publishers[id]
        }

        publishers[id] = {
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
        publishers[id].tabId = tab.id
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
      const id = getWindowId(payload.windowId)

      if (publisher && !publisher.publisher_key) {
        delete publishers[id]
      } else {
        publishers[id] = { ...publishers[id], ...publisher }
        const newPublisher = publishers[id]

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
    case types.GET_CURRENT_REPORT:
      chrome.braveRewards.getCurrentReport()
      break
    case types.ON_CURRENT_REPORT: {
      state = { ...state }
      state.report = payload.properties
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

      state = {
        ...state,
        notifications
      }

      setBadgeText(state)
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
      if (payload.enabledMain == null) {
        break
      }
      state.enabledMain = payload.enabledMain
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
          newPublisher.publisher_key === publisher.publisher_key)

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

      const publisherKey: string = payload.properties.publisher_key
      const excluded: boolean = payload.properties.excluded

      let publishers: Record<string, RewardsExtension.Publisher> = state.publishers

      for (const key in publishers) {
        let publisher = publishers[key]

        if (publisher.publisher_key === publisherKey) {
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
          if (publisher.publisher_key === publisherKey) {
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
        break
      }

      let notifications: Record<number, RewardsExtension.Notification> = state.notifications
      let id = ''

      // Array check for previous version of state types
      // (https://github.com/brave/brave-browser/issues/4344)
      if (!notifications || Array.isArray(notifications)) {
        notifications = {}
      }

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
        const id = getWindowId(tab.windowId)
        const publisher = publishers[id]

        if (!publisher || publisher.tabId !== tab.id) {
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
  }
  return state
}
