// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

export function getTopSiteTiles (): Promise<chrome.topSites.MostVisitedURL[]> {
  return new Promise(resolve => {
    window.cr.sendWithPromise<any>('getMostVisitedInfo').then((result) => {
      resolve(result.tiles)
    })
  })
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

export function generateGridSiteFavicon (url: string): string {
  return `chrome://favicon/size/64@1x/${url}`
}
