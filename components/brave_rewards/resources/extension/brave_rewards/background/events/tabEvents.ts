/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getTabData, onTabData } from '../api/tabs_api'

chrome.tabs.onUpdated.addListener((tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => {
  const activeTabIsLoadingTriggered = Boolean(tab.active && changeInfo.status === 'loading')
  onTabData(tab, activeTabIsLoadingTriggered)
})

chrome.tabs.onActivated.addListener((activeInfo: chrome.tabs.TabActiveInfo) => {
  getTabData(activeInfo.tabId)
})
