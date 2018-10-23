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

export const rewardsPanelReducer = (state: RewardsExtension.State | undefined, action: any) => {
  if (state === undefined) {
    state = storage.load()
    setBadgeText(state)
  }

  const startingState = state

  const payload = action.payload
  switch (action.type) {
    case types.CREATE_WALLET:
      chrome.braveRewards.createWallet()
      break
    case types.ON_WALLET_CREATED:
      state = { ...state }
      state.walletCreated = true
      break
    case types.ON_WALLET_CREATE_FAILED:
      state = { ...state }
      state.walletCreateFailed = true
      break
    case types.ON_TAB_ID:
      if (payload.tabId) {
        getTabData(payload.tabId)
      }
      break
    case types.ON_TAB_RETRIEVED:
      const tab: chrome.tabs.Tab = payload.tab
      if (
        !tab ||
        !tab.url ||
        tab.incognito ||
        !tab.active ||
        !state.walletCreated
      ) {
        break
      }

      state = { ...state }
      chrome.braveRewards.getPublisherData(tab.windowId, tab.url, tab.favIconUrl || '')
      break
    case types.ON_PUBLISHER_DATA:
      {
        const publisher = payload.publisher
        let publishers: Record<string, RewardsExtension.Publisher> = state.publishers
        const id = `id_${payload.windowId}`

        if (publisher && !publisher.publisher_key) {
          delete publishers[id]
        } else {
          publishers[id] = payload.publisher
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
    case types.ON_WALLET_PROPERTIES:
      {
        state = { ...state }
        state.walletProperties = payload.properties
        break
      }
    case types.GET_CURRENT_REPORT:
      chrome.braveRewards.getCurrentReport()
      break
    case types.ON_CURRENT_REPORT:
      {
        state = { ...state }
        state.report = payload.properties
        break
      }
    case types.ON_NOTIFICATION_ADDED:
      {
        if (!payload || !payload.id) {
          return
        }

        const id = `n_${payload.id}`
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
    case types.DELETE_NOTIFICATION:
      {
        const id = payload.id.toString().replace('n_', '')
        chrome.rewardsNotifications.deleteNotification(parseInt(id, 10))
        break
      }
    case types.ON_NOTIFICATION_DELETED:
      {
        if (!payload || !payload.id) {
          return
        }

        const id = `n_${payload.id}`
        let notifications: Record<number, RewardsExtension.Notification> = state.notifications
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
    case types.INCLUDE_IN_AUTO_CONTRIBUTION:
      {
        let publisherKey = payload.publisherKey
        let excluded = payload.excluded
        let windowId = payload.windowId
        chrome.braveRewards.includeInAutoContribution(publisherKey, excluded,
          windowId)
        break
      }
  }

  if (state !== startingState) {
    storage.debouncedSave(state)
  }

  return state
}
