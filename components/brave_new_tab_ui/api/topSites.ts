// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

let areCustomLinksEnabled: boolean = true
let areTopSitesVisible: boolean = true

export function updateMostVisitedInfo () {
  chrome.send('updateMostVisitedInfo')
}

export function addMostVistedInfoChangedListener (listener: any): void {
  window.cr.addWebUIListener('most-visited-info-changed', listener)
}

export function deleteMostVisitedTile (url: string): void {
  chrome.send('deleteMostVisitedTile', [url])
}

export function reorderMostVisitedTile (url: string, newPos: number): void {
  chrome.send('reorderMostVisitedTile', [url, newPos])
}

export function restoreMostVisitedDefaults (): void {
  chrome.send('restoreMostVisitedDefaults', [])
}

export function undoMostVisitedTileAction (): void {
  chrome.send('undoMostVisitedTileAction', [])
}

export function setMostVisitedSettings (customLinksEnabled: boolean,
    visible: boolean, updateInstantService: boolean): void {
  areCustomLinksEnabled = customLinksEnabled
  areTopSitesVisible = visible
  if (updateInstantService) {
    chrome.send('setMostVisitedSettings', [customLinksEnabled, visible])
  }
}

export function customLinksEnabled (): boolean {
  return areCustomLinksEnabled
}

export function isVisible (): boolean {
  return areTopSitesVisible
}
