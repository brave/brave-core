// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as Background from '../../../../../common/Background'
import { getPreference } from '../../../../../common/settingsPrivate'
import * as Feed from './feed'
import * as Publishers from './publishers'
import * as PublisherUserPrefs from './publisher-user-prefs'
import { fetchResource, getUnpaddedAsDataUrl } from './privateCDN'

const SETTINGS_KEY_SHOW_TODAY = 'brave.new_tab_page.show_brave_today'
const SETTINGS_KEY_OPTED_IN = 'brave.today.opted_in'
const ALARM_KEY_FEED_UPDATE = 'brave-today-update-feed'
const ALARM_KEY_PUBLISHERS_UPDATE = 'brave-today-update-publishers'

// Only do certain things when Brave Today is enabled
let isStartedUp = false

//
// This module is the orchestrator for the Brave Today backend.
// It handles command communication from front-ends, as well
// as setting up and handling timers, and events from the
// rest of the browser.
//

// Handle browser events
chrome.braveToday.onClearHistory.addListener(async () => {
  // Parsed and weighted feed items have somewhat history-related
  // data since they have a `score` property which has a different
  // value if the user has visited the article's URL host recently.
  // So, clear the generated scores (and the entire generated feed)
  // when the user clears history.
  console.debug('Clearing Brave Today feed from cache due to clear history event.')
  await Feed.clearCache()
})

// Setup alarms for refresh
function ensureUpdateFrequency () {
  chrome.alarms.get(ALARM_KEY_FEED_UPDATE, (alarm) => {
    if (!alarm) {
      chrome.alarms.create(ALARM_KEY_FEED_UPDATE, {
        delayInMinutes: 60 * 3,
        periodInMinutes: 60 * 3
      })
    }
  })
  chrome.alarms.get(ALARM_KEY_PUBLISHERS_UPDATE, (alarm) => {
    if (!alarm) {
      chrome.alarms.create(ALARM_KEY_PUBLISHERS_UPDATE, {
        delayInMinutes: 60 * 24,
        periodInMinutes: 60 * 24
      })
    }
  })
  chrome.alarms.onAlarm.addListener(async (alarm) => {
    switch (alarm.name) {
      case ALARM_KEY_FEED_UPDATE:
        await Feed.update()
        break
      case ALARM_KEY_PUBLISHERS_UPDATE:
        await Publishers.update()
        break
    }
  })
}

function stopUpdateFrequency () {
  chrome.alarms.clear(ALARM_KEY_FEED_UPDATE)
  chrome.alarms.clear(ALARM_KEY_PUBLISHERS_UPDATE)
}

// Setup listeners for messages from WebUI
import MessageTypes = Background.MessageTypes.Today
import Messages = BraveToday.Messages

async function fetchIsUpdateAvailableIfTime () {
  // When the user is interacting with Brave Today, we check
  // more frequently than our timer.
  const lastUpdateTime = await Feed.getLastUpdateCheckTime()
  const shouldCheck = !lastUpdateTime ||
    (Date.now() - lastUpdateTime) >= (10 * 60 * 1000) // 10 minutes
  if (shouldCheck) {
    const hasUpdate = await Feed.checkForRemoteUpdate()
    console.debug(`Brave Today checked for update because it's been used for 10 mins. Update ${hasUpdate ? 'was' : 'was NOT'} available.`)
    return hasUpdate
  }
  return null
}

Background.setListener<void>(
  MessageTypes.indicatingOpen,
  async function () {
    // This will only get called from a page that Brave Today is enabled on.
    // Start to fetch data if that hasn't happened yet,
    // but don't use resources returning it when
    // most NTP opens won't result in reading Brave Today
    // content (or changing publisher settings).
    // Only do this if we've previously interacted with brave today
    if (isStartedUp) {
      await Promise.all([
        Feed.getOrFetchData(),
        Publishers.getOrFetchData()
      ])
    }
  }
)

Background.setListener<Messages.GetFeedResponse>(
  MessageTypes.getFeed,
  async function (req, sender, sendResponse) {
    if (!isStartedUp) {
      sendResponse({ })
      return
    }
    try {
      const feed = await Feed.getOrFetchData()
      // Only wait once. If there was an error or no data then return nothing.
      sendResponse({
        feed
      })
    } catch (e) {
      sendResponse({ })
    }
  }
)

Background.setListener<Messages.GetPublishersResponse>(
  MessageTypes.getPublishers,
  async function (req, sender, sendResponse) {
    if (!isStartedUp) {
      sendResponse({ publishers: {} })
      return
    }
    // TODO(petemill): handle error
    const publishers = await Publishers.getOrFetchData()
    sendResponse({ publishers })
  }
)

Background.setListener<Messages.GetImageDataResponse, Messages.GetImageDataPayload>(
  MessageTypes.getImageData,
  async function (req, sender, sendResponse) {
    // TODO: handle error
    const blob = await fetchResource(req.url).then(r => r.blob())
    // @ts-ignore (Blob.arrayBuffer does exist)
    const buffer = await blob.arrayBuffer()
    const dataUrl = await getUnpaddedAsDataUrl(buffer, 'image/jpg')
    sendResponse({
      dataUrl
    })
  }
)

Background.setListener<{}, Messages.SetPublisherPrefPayload>(
  MessageTypes.setPublisherPref,
  async function (req, sender) {
    const publisherId = req.publisherId
    const enabled: boolean | null = req.enabled
    await PublisherUserPrefs.setPublisherPref(publisherId, enabled)
  }
)

Background.setListener<Messages.IsFeedUpdateAvailableResponse, Messages.IsFeedUpdateAvailablePayload>(
  MessageTypes.isFeedUpdateAvailable,
  async function (req, sender, sendResponse) {
    if (!isStartedUp) {
      sendResponse({ isUpdateAvailable: false })
      return
    }
    const requestHash = req.hash
    // Check for update from local
    const feed = await Feed.getOrFetchData(true)
    let isUpdateAvailable = !!(feed && feed.hash !== requestHash)
    if (!isUpdateAvailable) {
      // Maybe Check for update from remote
      const isRemoteUpdateAvailble = await fetchIsUpdateAvailableIfTime()
      isUpdateAvailable = isRemoteUpdateAvailble || false
    }
    sendResponse({ isUpdateAvailable })
  }
)

Background.setListener<
  Messages.ClearPrefsResponse, Messages.ClearPrefsPayload
>(MessageTypes.resetPrefsToDefault, async function (req, sender, sendResponse) {
  await PublisherUserPrefs.clearPrefs()
  const publishers = await Publishers.update(true)
  sendResponse({ publishers })
})

async function conditionallyStartupOrShutdown () {
  const prefs = await Promise.all([
    getPreference(SETTINGS_KEY_SHOW_TODAY),
    getPreference(SETTINGS_KEY_OPTED_IN)
  ])
  let isBraveTodayEnabled = true
  for (const pref of prefs) {
    if (pref.type !== chrome.settingsPrivate.PrefType.BOOLEAN) {
      throw new Error(`Unknown pref type for '${pref.key}'. Expected BOOLEAN but saw ${pref.type}.`)
    }
    if (!pref.value) {
      isBraveTodayEnabled = false
    }
  }
  if (isBraveTodayEnabled && !isStartedUp) {
    isStartedUp = true
    ensureUpdateFrequency()
  } else if (!isBraveTodayEnabled && isStartedUp) {
    stopUpdateFrequency()
    isStartedUp = false
  }
}

chrome.settingsPrivate.onPrefsChanged.addListener(async prefs => {
  await conditionallyStartupOrShutdown()
})

conditionallyStartupOrShutdown()
.catch((err) => {
  console.error('Could not startup Brave News', err)
})
