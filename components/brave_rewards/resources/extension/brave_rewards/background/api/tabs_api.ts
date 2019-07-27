/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const onTabData = (tab: chrome.tabs.Tab, activeTabIsLoadingTriggered?: boolean) => {
  const rewardsPanelActions = require('../actions/rewardsPanelActions').default

  if (tab.id && tab.active && tab.url) {
    const url = new URL(tab.url)
    if (url.hostname.endsWith('twitter.com')) {
      let screenName = url.pathname
      const screenNameParts = screenName.split('/')
      if (screenNameParts && screenNameParts.length > 1) {
        screenName = screenNameParts[1]
      }

      const msg = { type: 'getProfileUrl', screenName }

      chrome.tabs.sendMessage(tab.id, msg, response => {
        const profileUrl = response.profileUrl

        if (profileUrl) {
          tab.url = profileUrl
        }

        rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
      })
      return
    }
  }

  rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
}

export const getTabData = (tabId: number) =>
  chrome.tabs.get(tabId, (tab: chrome.tabs.Tab) => {
    onTabData(tab)
  })
