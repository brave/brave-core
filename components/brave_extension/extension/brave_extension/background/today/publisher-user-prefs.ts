// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import Events from '../../../../../common/events'

const STORAGE_KEY = 'todayPublisherUserPrefs'
const STORAGE_SCHEMA_VERSION = 1

type Prefs = {
  [publisherId: string]: boolean
}

let storageLock: Promise<void> | null = null
let cache: Prefs | undefined
const publisherPrefsEvents = new Events()
const eventNameChanged = 'publisher-prefs-changed'

function isValidStorageData(data: any) {
  // TODO(petemill): Since this is non-ephemeral data,
  // we should convert if schema version is not expected.
  return (
    data &&
    data[STORAGE_KEY] &&
    data[STORAGE_KEY].storageSchemaVersion === STORAGE_SCHEMA_VERSION &&
    data[STORAGE_KEY].prefs
  )
}

function setPrefsToStorage (prefs: Prefs) {
  return new Promise(resolve => {
      chrome.storage.sync.set({
      [STORAGE_KEY]: {
        storageSchemaVersion: STORAGE_SCHEMA_VERSION,
        prefs
      }
    }, resolve)
  })
}

function getPrefsFromStorage (): Promise<Prefs> {
  return new Promise(resolve => {
    chrome.storage.sync.get(STORAGE_KEY, (data) => {
      let prefs = {}
      if (isValidStorageData(data)) {
        prefs = data[STORAGE_KEY].prefs
      }
      resolve(prefs)
    })
  })
}

async function doWithLock<T>(todo: () => Promise<T>): Promise<T> {
  if (storageLock) {
    await storageLock
  }
  return new Promise(async (resolve) => {
    if (storageLock) {
      await storageLock
    }
    storageLock = new Promise(async resolveLock => {
      const result = await todo()
      resolve(result)
      storageLock = null
      resolveLock()
    })
  })
}

export async function getPrefs (): Promise<Prefs> {
  return doWithLock(async function () {
    const prefs = await getPrefsFromStorage()
    cache = prefs
    return prefs
  })
}

export async function setPublisherPref (publisherId: string, enabled: boolean | null) {
  return doWithLock(async function () {
    const prefs = await getPrefsFromStorage()
    if (enabled === null) {
      delete prefs[publisherId]
    } else {
      prefs[publisherId] = enabled
    }
    await setPrefsToStorage(prefs)
    cache = prefs
    setImmediate(() => {
      publisherPrefsEvents.dispatchEvent(eventNameChanged, cache)
    })
    return prefs
  })
}

type changeListener = (prefs: Prefs) => any
export function addPrefsChangedListener(listener: changeListener) {
  publisherPrefsEvents.addEventListener(eventNameChanged, listener)
}