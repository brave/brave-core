// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { isPublisherContentAllowed } from '../../../../../common/braveToday'
import { getOrFetchData as getOrFetchPublishers, addPublishersChangedListener } from './publishers'
import feedToData from './feedToData'
import { fetchResource, URLS } from './privateCDN'

type RemoteData = BraveToday.FeedItem[]

type FeedInStorage = {
  storageSchemaVersion: number,
  feed: BraveToday.Feed
}

const feedUrl = URLS.braveTodayFeed
let memoryTodayData: BraveToday.Feed | undefined
let readLock: Promise<void> | null
const STORAGE_KEY = 'today'
const STORAGE_KEY_ETAG = 'todayEtag'
const STORAGE_KEY_LAST_REMOTE_CHECK = 'todayLastRemoteUpdateCheck'
const STORAGE_SCHEMA_VERSION = 1
let isKnownRemoteUpdateAvailable = false

function getFromStorage<T> (key: string) {
  return new Promise<T | null>(resolve => {
    chrome.storage.local.get(key, (data) => {
      if (Object.keys(data).includes(key)) {
        resolve(data[key] as T)
      } else {
        resolve(null)
      }
    })
  })
}

function setStorageData (feed: BraveToday.Feed) {
  chrome.storage.local.set({
    [STORAGE_KEY]: {
      storageSchemaVersion: STORAGE_SCHEMA_VERSION,
      feed
    }
  })
}

function setStorageEtag (etag: string | null) {
  if (!etag) {
    chrome.storage.local.remove(STORAGE_KEY_ETAG)
  } else {
    chrome.storage.local.set({
      [STORAGE_KEY_ETAG]: etag
    })
  }
}

function getStorageEtag () {
  return getFromStorage<string>(STORAGE_KEY_ETAG)
}

function setLastUpdateCheckTime (time: number) {
  chrome.storage.local.set({
    [STORAGE_KEY_LAST_REMOTE_CHECK]: time
  })
}

export async function getLastUpdateCheckTime () {
  return getFromStorage<number>(STORAGE_KEY_LAST_REMOTE_CHECK)
}

async function getStorageData () {
  const data = await getFromStorage<FeedInStorage>(STORAGE_KEY)
  if (!data || !data.feed) {
    return
  }
  if (data.storageSchemaVersion !== STORAGE_SCHEMA_VERSION) {
    return
  }
  memoryTodayData = data.feed
}

function clearStorageData () {
  return new Promise(resolve => {
    chrome.storage.local.remove(STORAGE_KEY, resolve)
  })
}

// Immediately read from local storage and ensure we don't try
// to fetch whilst we're waiting.
const getLocalDataLock = getStorageData()

function performUpdateFeed () {
  // Sanity check
  if (readLock) {
    console.error('Asked to update feed but already waiting for another update!')
    return
  }
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve, reject) {
    try {
      const [feedResponse, publishers] = await Promise.all([
        // We don't use If-None-Match header because
        // often have to fetch same data and compute feed
        // with different publisher preferences.
        fetchResource(feedUrl),
        getOrFetchPublishers()
      ])
      if (feedResponse.ok) {
        // Save the Etag so we can check for updates periodically.
        const feedEtag = feedResponse.headers.get('Etag')
        setStorageEtag(feedEtag)
        setLastUpdateCheckTime(Date.now())
        const feedContents: RemoteData = await feedResponse.json()
        console.debug(`Successfully got new remote feed with etag ${feedEtag}.`, feedResponse.headers)
        if (!publishers) {
          throw new Error('no publishers to filter feed')
        }
        const enabledPublishers = {}
        for (const publisher of Object.values(publishers)) {
          if (isPublisherContentAllowed(publisher)) {
            enabledPublishers[publisher.publisher_id] = publisher
          }
        }
        memoryTodayData = await feedToData(feedContents, enabledPublishers)
        isKnownRemoteUpdateAvailable = false
        resolve()
        if (memoryTodayData) {
          setStorageData(memoryTodayData)
        }
      } else {
        throw new Error(`Not ok when fetching feed. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not process feed contents from ', feedUrl)
      reject(e)
    } finally {
      readLock = null
    }
  })
}

export async function getLocalData () {
  if (readLock) {
    await readLock
  }
  await getLocalDataLock
  return memoryTodayData
}

export async function getOrFetchData (waitForInProgressUpdate: boolean = false) {
  await getLocalDataLock
  if (waitForInProgressUpdate && readLock) {
    await readLock
  }
  if (memoryTodayData && !isKnownRemoteUpdateAvailable) {
    return memoryTodayData
  }
  return update()
}

export async function checkForRemoteUpdate () {
  // wait for any in-progress fetch
  if (isKnownRemoteUpdateAvailable) {
    return true
  }
  if (readLock) {
    await readLock
  }
  const existingEtag = await getStorageEtag()
  if (!existingEtag) {
    return true
  }
  const headResponse = await fetch(feedUrl, { method: 'HEAD' })
  if (!headResponse.ok) {
    console.warn(`Tried to fetch feed HEAD but got error ${headResponse.status}: "${headResponse.statusText}".`)
    return false
  }
  const newEtag = headResponse.headers.get('Etag')
  setLastUpdateCheckTime(Date.now())
  console.debug(`Brave Today checked for new update. New Etag is "${newEtag}". Last Etag was "${existingEtag}".`)
  if (newEtag && newEtag !== existingEtag) {
    isKnownRemoteUpdateAvailable = true
    return true
  }
  return false
}

export async function update (force: boolean = false) {
  // Fetch but only once at a time, and wait.
  if (!readLock) {
    performUpdateFeed()
  } else if (force) {
    // If there was already an update in-progress, and we want
    // to make sure we use the latest data, we'll have to perform
    // another update to be sure.
    await readLock
    performUpdateFeed()
  }
  await readLock
  return memoryTodayData
}

export async function clearCache () {
  await getLocalDataLock
  if (readLock) {
    await readLock
  }
  await clearStorageData()
  memoryTodayData = undefined
}

// When publishers (or publishers prefs) changes, update articles
addPublishersChangedListener(async function (publishers) {
  // Only update if we already have data
  if ((await getLocalData())) {
    await update(true)
  }
})
