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

chrome.browserAction.setBadgeBackgroundColor({ color: '#FB542B' }, () => {
  if (chrome.runtime.lastError) {
    console.warn('browserAction.setBadgeBackgroundColor failed: ' + chrome.runtime.lastError.message)
  }
})
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

chrome.runtime.onMessageExternal.addListener(
  function (msg: any, sender: chrome.runtime.MessageSender, sendResponse: any) {
    if (!msg) {
      return
    }
    switch (msg.type) {
      case 'OnPublisherData':
        rewardsPanelActions.onPublisherData(msg.tabId, msg.info)
        break
    }
  })
