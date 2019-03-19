/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../constants/rewards_panel_types'
import * as storage from '../storage'
import { getTabData } from '../api/tabs_api'

function setBadgeText (state: RewardsExtension.State): void {
  let text = ''

  if (state && state.notifications) {
    const count = Object.keys(state.notifications).length
    if (count > 0) {
      text = count.toString()
    }
  }

  chrome.browserAction.setBadgeText({
    text
  })
}

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
      break
    case types.ON_WALLET_CREATED:
      state = { ...state }
      state.walletCreated = true
      state.walletCreateFailed = false
      state.walletCreating = false
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
    case types.ON_WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      state.walletCreating = false
      state.walletCreated = false
      break
    case types.ON_TAB_ID:
      if (payload.tabId) {
        getTabData(payload.tabId)
      }
      break
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

      if (!publisher || (publisher && publisher.tabUrl !== tab.url)) {
        chrome.braveRewards.getPublisherData(
          tab.windowId,
          tab.url,
          tab.favIconUrl || '',
          payload.publisherBlob || '')

        if (publisher) {
          delete publishers[id]
        }

        publishers[id] = {
          tabUrl: tab.url
        }
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
      }

      state = {
        ...state,
        publishers
      }
      break
    }
    case types.GET_WALLET_PROPERTIES:
      chrome.braveRewards.getWalletProperties()
      break
    case types.ON_WALLET_PROPERTIES: {
      state = { ...state }
      state.walletProperties = payload.properties
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

      const id = payload.id
      let notifications: Record<number, RewardsExtension.Notification> = state.notifications

      if (!notifications) {
        notifications = []
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
      let excluded = payload.excluded
      chrome.braveRewards.includeInAutoContribution(publisherKey, excluded)
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
          publisher.verified = updated.verified
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
    case types.REMOVE_RECURRING_DONATION: {
      let publisherKey = payload.publisherKey
      if (publisherKey == null) {
        break
      }
      chrome.braveRewards.removeRecurringDonation(publisherKey)
      break
    }
    case types.SAVE_RECURRING_DONATION: {
      let newAmount = payload.newAmount
      let publisherKey = payload.publisherKey

      if (newAmount < 0 ||
          isNaN(newAmount) ||
          publisherKey == null) {
        break
      }

      chrome.braveRewards.saveRecurringDonation(publisherKey, newAmount)
      break
    }
    case types.ON_RECURRING_DONATIONS: {
      state = { ...state }
      state.recurringDonations = payload.result.recurringDonations
      break
    }
    case types.ON_PUBLISHER_BANNER: {
      if (!payload.banner || !payload.banner.publisherKey) {
        break
      }

      state = { ...state }
      if (!state.donationAmounts) {
        state.donationAmounts = {}
      }
      state.donationAmounts[payload.banner.publisherKey] = payload.banner.amounts
      break
    }
  }
  return state
}
