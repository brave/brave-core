// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import Events from '../../../../../common/events'

const PREF_KEY = 'brave.today.sources'

type Prefs = {
  [publisherId: string]: boolean
}

let storageLock: Promise<void> | null = null
const publisherPrefsEvents = new Events()
const eventNameChanged = 'publisher-prefs-changed'

chrome.settingsPrivate.onPrefsChanged.addListener((prefs) => {
  const pref = prefs.find(p => p.key === PREF_KEY)
  if (pref) {
    console.debug('today sources pref changed', pref.value)
    const prefValue = pref.value as Prefs
    setImmediate(() => {
      publisherPrefsEvents.dispatchEvent(eventNameChanged, prefValue)
    })
  }
})

function setPrefsToStore (prefs: Prefs) {
  return new Promise(resolve => {
    chrome.settingsPrivate.setPref(PREF_KEY, prefs, resolve)
  })
}

function getPrefsFromStore (): Promise<Prefs> {
  return new Promise(resolve => {
    chrome.settingsPrivate.getPref(PREF_KEY, (pref) => {
      if (pref && pref.type === chrome.settingsPrivate.PrefType.DICTIONARY) {
        resolve(pref.value as Prefs)
      }
    })
  })
}

async function doWithLock<T> (todo: () => Promise<T>): Promise<T> {
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
    const prefs = await getPrefsFromStore()
    return prefs
  })
}

export async function setPublisherPref (publisherId: string, enabled: boolean | null) {
  return doWithLock(async function () {
    const prefs = await getPrefsFromStore()
    if (enabled === null) {
      delete prefs[publisherId]
    } else {
      prefs[publisherId] = enabled
    }
    await setPrefsToStore(prefs)
    // Update cache in case we request an update before
    // we get a notification from the prefs change handler.
    return prefs
  })
}

export async function clearPrefs () {
  return doWithLock(async function () {
    await setPrefsToStore({})
  })
}

type changeListener = (prefs: Prefs) => any
export function addPrefsChangedListener (listener: changeListener) {
  publisherPrefsEvents.addEventListener(eventNameChanged, listener)
}
