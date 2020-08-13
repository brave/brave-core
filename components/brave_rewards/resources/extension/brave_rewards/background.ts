/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import rewardsPanelActions from './background/actions/rewardsPanelActions'

import './background/publisherVisit'
import './background/store'
import './background/twitterAuth'
import './background/events/rewardsEvents'
import './background/events/tabEvents'

import { processPendingTabs } from './background/api/tabs_api'
// tslint:disable-next-line:no-duplicate-imports
import { setTwitterAuthCallback } from './background/twitterAuth'

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

setTwitterAuthCallback(processPendingTabs)

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

const tipTwitterMedia = (mediaMetaData: RewardsTip.MediaMetaData) => {
  mediaMetaData.mediaType = 'twitter'
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
    chrome.braveRewards.tipTwitterUser(tabId, mediaMetaData)
  })
}

const tipGitHubMedia = (mediaMetaData: RewardsTip.MediaMetaData) => {
  mediaMetaData.mediaType = 'github'
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
    chrome.braveRewards.tipGitHubUser(tabId, mediaMetaData)
  })
}

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

const makeTwitterRequest = (url: string, credentialHeaders: {}, sendResponse: any) => {
  fetch(url, {
    credentials: 'include',
    headers: {
      ...credentialHeaders
    },
    referrerPolicy: 'no-referrer-when-downgrade',
    method: 'GET',
    redirect: 'follow'
  })
    .then(response => {
      if (!response.ok) {
        throw new Error(`Twitter API request failed: ${response.statusText} (${response.status})`)
      }

      return response.json()
    })
    .then(data => sendResponse(data))
    .catch(error => {
      sendResponse(error)
    })
}

chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
  const action = typeof msg === 'string' ? msg : msg.type
  switch (action) {
    case 'tipInlineMedia': {
      switch (msg.mediaMetaData.mediaType) {
        case 'twitter':
          tipTwitterMedia(msg.mediaMetaData)
          break
        case 'reddit':
          tipRedditMedia(msg.mediaMetaData)
          break
        case 'github':
          tipGitHubMedia(msg.mediaMetaData)
          break
      }
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
    case 'inlineTippingPlatformEnabled': {
      // Check if inline tip is enabled
      chrome.braveRewards.getInlineTippingPlatformEnabled(msg.key, function (enabled: boolean) {
        sendResponse({ enabled })
      })
      // Must return true for asynchronous calls to sendResponse
      return true
    }
    case 'twitterGetTweetDetails': {
      const url = new URL('https://api.twitter.com/1.1/statuses/show.json')
      url.searchParams.append('id', msg.tweetId)
      makeTwitterRequest(url.toString(), msg.credentialHeaders, sendResponse)
      return true
    }
    case 'twitterGetUserDetails': {
      const url = new URL('https://api.twitter.com/1.1/users/show.json')
      url.searchParams.append('screen_name', msg.screenName)
      makeTwitterRequest(url.toString(), msg.credentialHeaders, sendResponse)
      return true
    }
    default:
      return false
  }
})
