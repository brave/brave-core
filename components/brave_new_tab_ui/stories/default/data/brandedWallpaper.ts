// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import wallpaperImageUrl from '../../../../img/newtab/dummy-branded-wallpaper/background-1.jpg'
import brandingImageUrl from '../../../../img/newtab/dummy-branded-wallpaper/logo.png'
import { NewTabPageAdMetricType } from 'gen/brave/components/brave_ads/core/mojom/brave_ads.mojom.m.js'

const dummyWallpaper: NewTab.BrandedWallpaper = {
  type: 'image',
  wallpaperImageUrl,
  isSponsored: true,
  creativeInstanceId: '12345abcde',
  metricType: NewTabPageAdMetricType.kConfirmation,
  wallpaperId: 'abcde12345',
  logo: {
    image: brandingImageUrl,
    companyName: 'Technikke',
    alt: 'Technikke: For music lovers.',
    destinationUrl: 'https://brave.com'
  }
}

export default dummyWallpaper
