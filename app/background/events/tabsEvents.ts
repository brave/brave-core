/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import tabActions from '../actions/tabActions'

chrome.tabs.onActivated.addListener((activeInfo: chrome.tabs.TabActiveInfo) => {
  tabActions.activeTabChanged(activeInfo.windowId, activeInfo.tabId)
})

chrome.tabs.onCreated.addListener(function (tab: chrome.tabs.Tab) {
  tabActions.tabCreated(tab)
})

chrome.tabs.onUpdated.addListener(function (tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) {
  tabActions.tabDataChanged(tabId, changeInfo, tab)
})
