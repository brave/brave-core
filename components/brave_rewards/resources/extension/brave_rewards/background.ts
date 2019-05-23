/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from './background/actions/rewardsPanelActions'

import './background/store'
import './background/events/rewardsEvents'
import './background/events/tabEvents'
import batIconOn18Url from './img/rewards-on.png'
import batIconOn36Url from './img/rewards-on@2x.png'
import batIconOn54Url from './img/rewards-on@3x.png'

const iconOn = {
  path: {
    18: batIconOn18Url,
    36: batIconOn36Url,
    54: batIconOn54Url
  }
}

chrome.browserAction.setBadgeBackgroundColor({ color: '#FB542B' })
chrome.browserAction.setIcon(iconOn)

// We need to set initial state for all active tabs in all windows
chrome.tabs.query({
  highlighted: true
}, (tabs) => {
  if (!tabs || !tabs.length) {
    return
  }

  rewardsPanelActions.init(tabs)
})

chrome.runtime.onInstalled.addListener(function (details) {
  if (details.reason === 'install') {
    const initialNotificationDismissed = 'false'
    chrome.storage.local.set({
      'is_dismissed': initialNotificationDismissed
    }, function () {
      chrome.browserAction.setBadgeText({
        text: '1'
      })
    })
  }
})

chrome.runtime.onStartup.addListener(function () {
  chrome.storage.local.get(['is_dismissed'], function (result) {
    if (result && result['is_dismissed'] === 'false') {
      chrome.browserAction.setBadgeText({
        text: '1'
      })
    }
  })

  chrome.runtime.onConnect.addListener(function (externalPort) {
    chrome.storage.local.set({
      'rewards_panel_open': 'true'
    })

    externalPort.onDisconnect.addListener(function () {
      chrome.storage.local.set({
        'rewards_panel_open': 'false'
      })
    })
  })
})

chrome.runtime.onConnect.addListener(function () {
  chrome.storage.local.get(['is_dismissed'], function (result) {
    if (result && result['is_dismissed'] === 'false') {
      chrome.browserAction.setBadgeText({
        text: ''
      })
      chrome.storage.local.remove(['is_dismissed'])
    }
  })
})

const tipTwitterUser = (tweetMetaData: RewardsTip.TweetMetaData) => {
  chrome.tabs.query({
    active: true,
    windowId: chrome.windows.WINDOW_ID_CURRENT
  }, (tabs) => {
    if (!tabs || tabs.length === 0) {
      return
    }
    const tabId = tabs[0].id
    if (tabId === undefined) {
      return
    }
    chrome.braveRewards.tipTwitterUser(tabId, tweetMetaData)
  })
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'tipTwitterUser': {
      tipTwitterUser(msg.tweetMetaData)
      return false
    }
    case 'rewardsEnabled': {
      // Check if rewards is enabled
      chrome.braveRewards.getRewardsMainEnabled(function (enabled: boolean) {
        sendResponse({ enabled })
      })
      // Must return true for asynchronous calls to sendResponse
      return true
    }
    case 'inlineTipSetting': {
      // Check if inline tip is enabled
      chrome.braveRewards.getInlineTipSetting(msg.key, function (enabled: boolean) {
        sendResponse({ enabled })
      })
      // Must return true for asynchronous calls to sendResponse
      return true
    }
    default:
      return false
  }
})
