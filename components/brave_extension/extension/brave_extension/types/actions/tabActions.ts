/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/tabTypes'

interface ActiveTabChangedReturn {
  type: types.ACTIVE_TAB_CHANGED
  windowId: number
  tabId: number
}

export type ActiveTabChanged = (windowId: number, tabId: number) => ActiveTabChangedReturn

interface TabCreatedReturn {
  type: types.TAB_CREATED
  tab: chrome.tabs.Tab
}

export type TabCreated = (tab: chrome.tabs.Tab) => TabCreatedReturn

interface TabDataChangedReturn {
  type: types.TAB_DATA_CHANGED
  tabId: number
  changeInfo: chrome.tabs.TabChangeInfo
  tab: chrome.tabs.Tab
}

export type TabDataChanged = (tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => TabDataChangedReturn

export type tabActions =
  ActiveTabChangedReturn |
  TabCreatedReturn |
  TabDataChangedReturn
