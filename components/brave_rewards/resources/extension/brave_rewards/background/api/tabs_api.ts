/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

interface PendingTab {
  tab: chrome.tabs.Tab
  activeTabIsLoadingTriggered?: boolean
}

let pendingTabs: { [url: string]: PendingTab } = {}

const isTwitterURL = (url: URL) => {
  return url && (url.hostname.endsWith('.twitter.com') || url.hostname === 'twitter.com')
}

const getTwitterScreenName = (url: URL) => {
  let screenName = url.pathname
  const screenNameParts = screenName.split('/')
  if (screenNameParts && screenNameParts.length > 1) {
    screenName = screenNameParts[1]
  }
  return screenName
}

export const onTabData = (tab: chrome.tabs.Tab, activeTabIsLoadingTriggered?: boolean) => {
  const rewardsPanelActions = require('../actions/rewardsPanelActions').default

  if (tab.id && tab.active && tab.url) {
    const url = new URL(tab.url)
    if (isTwitterURL(url)) {
      const screenName = getTwitterScreenName(url)
      if (screenName) {
        if (localStorage.getItem('twitterAuthHeaders') === null) {
          pendingTabs[tab.url] = { tab, activeTabIsLoadingTriggered }
          return
        }
        const msg = { type: 'getProfileUrl', screenName }
        chrome.tabs.sendMessage(tab.id, msg, response => {
          if (response && response.profileUrl && tab.url !== response.profileUrl) {
            tab.url = response.profileUrl
          }

          rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
        })
      }
      return
    }
  }

  rewardsPanelActions.onTabRetrieved(tab, activeTabIsLoadingTriggered)
}

export const processPendingTabs = () => {
  if (Object.keys(pendingTabs).length === 0) {
    return
  }
  chrome.tabs.query({
    active: true,
    windowId: chrome.windows.WINDOW_ID_CURRENT
  }, (tabs) => {
    if (!tabs || tabs.length === 0) {
      return
    }
    for (let key in pendingTabs) {
      const pendingTab = pendingTabs[key]
      if (pendingTab.tab.url === tabs[0].url) {
        onTabData(pendingTab.tab, pendingTab.activeTabIsLoadingTriggered)
        break
      }
    }
    pendingTabs = {}
  })
}

export const getTabData = (tabId: number) =>
  chrome.tabs.get(tabId, (tab: chrome.tabs.Tab) => {
    onTabData(tab)
  })
