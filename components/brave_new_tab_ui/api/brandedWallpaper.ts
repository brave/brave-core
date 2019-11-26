// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import wallpaperImageUrl from '../data/dummy-branded-wallpaper/background.jpg'
import brandingImageUrl from '../data/dummy-branded-wallpaper/logo.png'
export function getBrandedWallpaper (): Promise<NewTab.BrandedWallpaper> {
  return Promise.resolve({
    wallpaperImageUrl,
    logo: {
      image: brandingImageUrl,
      alt: 'Technikke: For music lovers.',
      destinationUrl: 'https://brave.com'
    }
  })
}

const viewsTillShouldShowStorageKey = 'NTP.BrandedWallpaper.ViewsTillShouldShow'
const maxViewCountUntilBranded = 4

export function getShouldShow (): boolean {
  const valueRaw = localStorage.getItem(viewsTillShouldShowStorageKey)
  const value = valueRaw ? parseInt(valueRaw) : NaN
  if (Number.isInteger(value)) {
    return value === 1
  }
  return false
}

export function registerViewCount (): void {
  // TODO: keep this value as singleton in C++ since we want it to show on the
  // 2nd view (after wallpapers and branded wallpapers are enabled) and then
  // every 4 views
  const valueRaw = localStorage.getItem(viewsTillShouldShowStorageKey)
  const value = valueRaw ? parseInt(valueRaw) : NaN
  let newValue = maxViewCountUntilBranded
  if (Number.isInteger(value)) {
    if (value > 1 && value <= maxViewCountUntilBranded) {
      newValue = value - 1
    }
  }
  localStorage.setItem(viewsTillShouldShowStorageKey, newValue.toString())
}
