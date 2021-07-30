// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { sendWithPromise } from '../../common/cr'

export function getBrandedWallpaper (): Promise<undefined | NewTab.BrandedWallpaper> {
  return sendWithPromise<undefined | NewTab.BrandedWallpaper>('getBrandedWallpaperData')
}

export function registerViewCount (): Promise<void> {
  return sendWithPromise<void>('registerNewTabPageView')
}

export function brandedWallpaperLogoClicked (data: NewTab.BrandedWallpaper | undefined) {
  chrome.send('brandedWallpaperLogoClicked', [ data ])
}
