/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const onTabData = (tab: chrome.tabs.Tab, activeTabIsLoadingTriggered?: boolean) => {
  const rewardsPanelActions = require('../actions/rewardsPanelActions').default
  rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
}

export const getTabData = (tabId: number) =>
  chrome.tabs.get(tabId, (tab: chrome.tabs.Tab) => {
    onTabData(tab)
  })
