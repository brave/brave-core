/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import './background/store'
import './background/events/rewardsEvents'
import './background/events/tabEvents'

chrome.browserAction.setBadgeBackgroundColor({ color: '#FF0000' })

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

//
// Detect User Tip requests via network calls
//
// Twitter
//
const requestFilter = {
  urls: ['https://api.twitter.com/1.1/favorites/create.json']
}

const twitterContentScriptArgs = {
  file: '/out/brave_rewards_content_twitter.bundle.js'
}

async function onTwitterFavoriteCreated (details: any) {
  const logPrefix = 'USER_TIP [twitter]: '
  if (details.method !== 'POST' || !details.requestBody || !details.requestBody.formData) {
    console.error(`${logPrefix}Received a favorite request, but not a POST, was a `, details.method)
    return
  }
  const { formData } = details.requestBody
  if (!formData.id || !formData.id.length) {
    console.error(`${logPrefix}Received a favorite request, but no item ID`, details.formData)
    return
  }
  const tweetId = formData.id[0]
  console.log(`${logPrefix}Tab [${details.tabId}] received a favorite request for tweet`, tweetId)
  const tabMessagePayload = { userAction: 'USER_TIP', tweetId }
  // If we've already added the content script send the message immediately
  chrome.tabs.sendMessage(details.tabId, tabMessagePayload, (tabResponsePayload?: any) => {
    // If the content script isn't loaded, then we won't get a payload back
    // and chrome.runtime.lastError will inform us that there is no content script
    // in the frame yet.
    if (!tabResponsePayload || !tabResponsePayload.received) {
      // Inject the content script for this tab frame
      chrome.tabs.executeScript(details.tabId, twitterContentScriptArgs, () => {
        // Content script loaded. Send the message again.
        chrome.tabs.sendMessage(details.tabId, tabMessagePayload)
      })
    }
  })
}

// onBeforeRequest is the only extension API we can access req / res body data
chrome.webRequest.onBeforeRequest.addListener(onTwitterFavoriteCreated, requestFilter, ['requestBody'])
