// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { URLS } from './privateCDN'

let memoryData: BraveToday.Publishers | undefined
let readLock: Promise<void> | null
const url = URLS.braveTodayPublishers

const getLocalDataLock = new Promise<void>(resolve => {
  chrome.storage.local.get('todayPublishers', (data) => {
    if (data && data.today) {
      memoryData = data.todayPublishers
    }
    resolve()
  })
})

function convertToObject(publishers: BraveToday.Publisher[]): BraveToday.Publishers {
  const data: BraveToday.Publishers = {}
  for (const publisher of publishers) {
    data[publisher.publisher_id] = {
      ...publisher,
      // TODO: read existing value
      user_enabled: null
    }
  }
  return data
}

function performUpdate() {
  // Sanity check
  if (readLock) {
    console.error('Asked to update feed but already waiting for another update!')
    return
  }
  // Only run this once at a time, otherwise wait for the update
  readLock = new Promise(async function (resolve, reject) {
    try {
      const feedResponse = await fetch(url)
      if (feedResponse.ok) {
        const feedContents: BraveToday.Publisher[] = await feedResponse.json()
        console.debug('fetched today publishers', feedContents)
        memoryData = convertToObject(feedContents)
        resolve()
        chrome.storage.local.set({ todayPublishers: memoryData })
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

// export function setPublisherEnabled(enable: boolean) {
//   // Change stored value
//   // Update list
//   // Update feed
// }

export async function getOrFetchData() {
  await getLocalDataLock
  if (memoryData) {
    return memoryData
  }
  return await update()
}

export async function update() {
  // Fetch but only once at a time, and wait.
  if (!readLock) {
    performUpdate()
  }
  await readLock
  return memoryData
}