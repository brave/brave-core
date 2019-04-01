/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from '../actions/rewardsPanelActions'

chrome.tabs.onUpdated.addListener((tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => {
  rewardsPanelActions.onTabRetrieved(tab)
})

chrome.tabs.onActivated.addListener((activeInfo: chrome.tabs.TabActiveInfo) => {
  rewardsPanelActions.onTabId(activeInfo.tabId)
})

const twitchHTML = (details: chrome.webNavigation.WebNavigationFramedCallbackDetails) => {
  chrome.tabs.executeScript(details.tabId, {
    code: 'document.body.outerHTML'
  }, function (result: string[]) {
    if (Array.isArray(result)) {
      chrome.tabs.get(details.tabId, (tab: chrome.tabs.Tab) => {
        if (!tab) {
          return
        }

        if (tab.id === details.tabId) {
          rewardsPanelActions.onTabRetrieved(tab, result[0])
        }
      })
    }
  })
}

chrome.webNavigation.onCompleted.addListener(twitchHTML, {
  url: [
    {
      hostSuffix: 'twitch.tv'
    }
  ]
})

chrome.webNavigation.onHistoryStateUpdated.addListener((details: any) => {
  console.log('onHistoryStateUpdated')
  if (details.transitionType === 'link') {
    twitchHTML(details)
  }
}, {
  url: [
    {
      hostSuffix: 'twitch.tv'
    }
  ]
})
