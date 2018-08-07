/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/tab_types'

export const tabCreated = (tab: chrome.tabs.Tab) => action(types.TAB_CREATED, {
  tab
})

export const tabUpdated = (tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) =>
  action(types.TAB_UPDATED, {
    tabId, changeInfo, tab
})

export const tabRemoved = (tabId: number) => action(types.TAB_REMOVED, {
  tabId
})

export const activeTabChanged = (tabId: number, windowId: number) => action(types.ACTIVE_TAB_CHANGED, {
  tabId, windowId
})
