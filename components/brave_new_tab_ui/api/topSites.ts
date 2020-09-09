// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

let are_custom_links_enabled: boolean
let is_visible: boolean

export function updateMostVisitedInfo () {
  chrome.send('updateMostVisitedInfo')
}

export function addMostVistedInfoChangedListener (listener: any): void {
  window.cr.addWebUIListener('most-visited-info-changed', listener)
}

export function deleteMostVisitedTile (url: string): void {
  chrome.send('deleteMostVisitedTile', [url])
}

export function reorderMostVisitedTile (url: string, new_pos: any): void {
  chrome.send('reorderMostVisitedTile', [url, new_pos])
}

export function restoreMostVisitedDefaults (): void {
  chrome.send('restoreMostVisitedDefaults', [])
}

export function undoMostVisitedTileAction (): void {
  chrome.send('undoMostVisitedTileAction', [])
}

export function setMostVisitedSettings (custom_links_enabled: boolean,
    visible: boolean): void {
  are_custom_links_enabled = custom_links_enabled
  is_visible = visible
  chrome.send('setMostVisitedSettings', [custom_links_enabled, visible])
}

export function customLinksEnabled (): boolean {
  return are_custom_links_enabled
}

export function isVisible (): boolean {
  return is_visible
}

export function generateGridSiteFavicon (site: NewTab.Site): string {
  if (site.favicon === '')
    return `chrome://favicon/size/64@1x/${site.url}`
  return site.favicon
}


/*

TODOS:
2. Wire up the top sites visible to Chromium one & deprecate pref
  c. when reading value
3. Fix the hover style; only should show an X in the top right. Nothing else.
*/
