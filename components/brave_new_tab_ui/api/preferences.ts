// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

//
// Manages get and set of NTP preference data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

type Preferences = {
  showBackgroundImage: boolean
  showStats: boolean
  showClock: boolean
  showTopSites: boolean
}

function getWebUIBooleanVal(key: string): boolean {
  return (chrome.getVariableValue(key).toLowerCase() === 'true')
}

export function getPreferences (): Promise<Preferences> {
  // Note(petemill): Returning as promise allows this
  // to be async even though it isn't right now.
  // Enforces practice of not setting directly
  // in a redux reducer.
  return Promise.resolve({
    showBackgroundImage: getWebUIBooleanVal('showBackgroundImage'),
    showStats: getWebUIBooleanVal('showStats'),
    showClock: getWebUIBooleanVal('showClock'),
    showTopSites: getWebUIBooleanVal('showTopSites')
  })
}

function sendSavePref(key: string, value: any) {
  chrome.send('saveNewTabPagePref', [key, value])
}

export function saveShowBackgroundImage (value: boolean): void {
  sendSavePref('showBackgroundImage', value)
}

export function saveShowClock (value: boolean): void {
  sendSavePref('showClock', value)
}

export function saveShowTopSites (value: boolean): void {
  sendSavePref('showTopSites', value)
}

export function saveShowStats (value: boolean): void {
  sendSavePref('showStats', value)
}

export function addChangeListener (listener: () => void): void {
  window.cr.addWebUIListener('preferences-changed', listener)
}
