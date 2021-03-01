// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export type MostVisitedInfoChanged = {
  tiles: NewTab.Site[]
  custom_links_enabled: boolean
  visible: boolean
}

export type MostVisitedInfoChangedHandler = (data: MostVisitedInfoChanged) => any

export function updateMostVisitedInfo () {
  chrome.send('updateMostVisitedInfo')
}

export function addMostVisitedInfoChangedListener (listener: MostVisitedInfoChangedHandler): void {
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
    visible: boolean): void {
  chrome.send('setMostVisitedSettings', [customLinksEnabled, visible])
}
