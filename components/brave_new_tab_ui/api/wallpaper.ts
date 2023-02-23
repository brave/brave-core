// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { sendWithPromise } from 'chrome://resources/js/cr.js'

export function getWallpaper (): Promise<undefined | NewTab.Wallpaper> {
  return sendWithPromise('getWallpaperData')
}

export function registerViewCount (): Promise<void> {
  return sendWithPromise('registerNewTabPageView')
}

export function brandedWallpaperLogoClicked (data: NewTab.BrandedWallpaper | undefined) {
  chrome.send('brandedWallpaperLogoClicked', [data])
}
