// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

//
// Manages get and set of NTP preference data
// Ensures everything to do with communication
// with the WebUI backend is all in 1 place,
// especially string keys.
//

export type Preferences = {
  showBackgroundImage: boolean
  brandedWallpaperOptIn: boolean
  showStats: boolean
  showClock: boolean
  showTopSites: boolean
  showRewards: boolean
  isBrandedWallpaperNotificationDismissed: boolean
}

type PreferencesUpdatedHandler = (prefData: Preferences) => void

export function getPreferences (): Promise<Preferences> {
  return window.cr.sendWithPromise<Preferences>('getNewTabPagePreferences')
}

function sendSavePref (key: string, value: any) {
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

export function saveShowRewards (value: boolean): void {
  sendSavePref('showRewards', value)
}

export function saveShowTogether (value: boolean): void {
  sendSavePref('showTogether', value)
}

export function saveShowBinance (value: boolean): void {
  sendSavePref('showBinance', value)
}

export function saveBrandedWallpaperOptIn (value: boolean): void {
  sendSavePref('brandedWallpaperOptIn', value)
}

export function saveIsBrandedWallpaperNotificationDismissed (value: boolean): void {
  sendSavePref('isBrandedWallpaperNotificationDismissed', value)
}

export function addChangeListener (listener: PreferencesUpdatedHandler): void {
  window.cr.addWebUIListener('preferences-changed', listener)
}
