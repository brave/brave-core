/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import tabActions from '../actions/tabActions'

chrome.tabs.onCreated.addListener((tab: chrome.tabs.Tab) => {
  tabActions.tabCreated(tab)
})

chrome.tabs.onUpdated.addListener((tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => {
  tabActions.tabUpdated(tabId, changeInfo, tab)
})

chrome.tabs.onRemoved.addListener((tabId: number, removeInfo: chrome.tabs.TabRemoveInfo) => {
  tabActions.tabRemoved(tabId)
})

chrome.tabs.onActivated.addListener((activeInfo: chrome.tabs.TabActiveInfo) => {
  tabActions.activeTabChanged(activeInfo.tabId, activeInfo.windowId)
})
