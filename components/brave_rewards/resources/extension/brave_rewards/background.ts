/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from './background/actions/rewardsPanelActions'

import './background/publisherVisit'
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

chrome.runtime.onStartup.addListener(function () {
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

const tipRedditMedia = (mediaMetaData: RewardsTip.MediaMetaData) => {
  mediaMetaData.mediaType = 'reddit'
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
    chrome.braveRewards.tipRedditUser(tabId, mediaMetaData)
  })
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'tipInlineMedia': {
      switch (msg.mediaMetaData.mediaType) {
        case 'reddit':
          tipRedditMedia(msg.mediaMetaData)
          break
      }
      return false
    }

    case 'inlineTippingPlatformEnabled': {
      // Check if inline tip is enabled
      chrome.braveRewards.getInlineTippingPlatformEnabled(msg.key, function (enabled: boolean) {
        sendResponse({ enabled })
      })
      // Must return true for asynchronous calls to sendResponse
      return true
    }
    default:
      return false
  }
})
