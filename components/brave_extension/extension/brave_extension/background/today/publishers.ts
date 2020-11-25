// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import Events from '../../../../../common/events'
import { fetchResource, URLS } from './privateCDN'
import { getPrefs as getPublisherPrefs, addPrefsChangedListener } from './publisher-user-prefs'

let memoryData: BraveToday.Publishers | undefined
let readLock: Promise<void> | null
const url = URLS.braveTodayPublishers
const publishersEvents = new Events()
const eventNameChanged = 'publishers-changed'
const storageKey = 'todayPublishers'
const STORAGE_SCHEMA_VERSION = 1

function isValidStorageData (data: {[key: string]: any}) {
  return (
    data && data.todayPublishers &&
    data.todayPublishers.storageSchemaVersion === STORAGE_SCHEMA_VERSION &&
    data.todayPublishers.publishers
  )
}

function setPublishersCache (publishers: BraveToday.Publishers) {
  chrome.storage.local.set({
    [storageKey]: {
      storageSchemaVersion: STORAGE_SCHEMA_VERSION,
      publishers
    }
  })
}

function getPublishersFromCache () {
  return new Promise<void>(resolve => {
    chrome.storage.local.get(storageKey, (data) => {
      if (isValidStorageData(data)) {
        memoryData = data[storageKey].publishers
      }
      resolve()
    })
  })
}

const getLocalDataLock: Promise<void> = getPublishersFromCache()

async function convertToObject (publishers: BraveToday.Publisher[]): Promise<BraveToday.Publishers> {
  const prefs = await getPublisherPrefs()
  const data: BraveToday.Publishers = {}
  for (const publisher of publishers) {
    data[publisher.publisher_id] = {
      ...publisher,
      // TODO: read existing value
      user_enabled: prefs[publisher.publisher_id]
    }
  }
  return data
}

function performUpdate () {
  // Sanity check
  if (readLock) {
    console.error('Asked to update feed but already waiting for another update!')
    return
  }
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve, reject) {
    try {
      // TODO(petemill): Use If-None-Match so we don't re-download the exact
      // same publisher list. Save Etag in storage.
      const feedResponse = await fetchResource(url)
      if (feedResponse.ok) {
        const feedContents: BraveToday.Publisher[] = await feedResponse.json()
        console.debug('fetched today publishers', feedContents)
        memoryData = await convertToObject(feedContents)
        resolve()
        // Notify
        setPublishersCache(memoryData)
        publishersEvents.dispatchEvent<BraveToday.Publishers>(eventNameChanged, memoryData)
      } else {
        throw new Error(`Not ok when fetching publishers. Status ${feedResponse.status} (${feedResponse.statusText})`)
      }
    } catch (e) {
      console.error('Could not process publishers contents from ', url)
      reject(e)
    } finally {
      readLock = null
    }
  })
}

export async function getOrFetchData () {
  await getLocalDataLock
  if (memoryData) {
    return memoryData
  }
  return update()
}

export async function update (force: boolean = false) {
  // Fetch but only once at a time, and wait.
  if (!readLock) {
    performUpdate()
  } else if (force) {
    // If there was already an update in-progress, and we want
    // to make sure we use the latest data, we'll have to perform
    // another update to be sure.
    await readLock
    performUpdate()
  }
  await readLock
  return memoryData
}

// Allow subscribers to observe when we have new data
type changeListener = (publishers: BraveToday.Publishers) => any
export function addPublishersChangedListener (listener: changeListener) {
  publishersEvents.addEventListener<BraveToday.Publishers>(eventNameChanged, listener)
}

// When publisher pref changes, update prefs data as we depend on it
addPrefsChangedListener(async function (prefs) {
  // TODO(petemill): only re-parse, don't re-fetch (as no indication that remote data
  // has changed).
  await update(true)
})
